#include "MainComponent.h"
#include "../../Common/SoteroArchive.h"
#include "../../SamplerPlayer/Source/SoteroSamplerVoice.h"

namespace sotero {
MainComponent::MainComponent()
    : apvts(std::make_unique<juce::AudioProcessorValueTreeState>(
          dummyProcessor, nullptr, "Parameters", createParameterLayout())) {
  libraryData.mappings.clear();

  editView = std::make_unique<juce::Component>();

  // --- BUILDERUI (Edit View) ---
  editView->addAndMakeVisible(titleLabel);
  titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
  titleLabel.setJustificationType(juce::Justification::centred);

  nameEditor.setTextToShowWhenEmpty("Library Name", juce::Colours::grey);
  authorEditor.setTextToShowWhenEmpty("Author Name", juce::Colours::grey);
  editView->addAndMakeVisible(nameEditor);
  editView->addAndMakeVisible(authorEditor);

  for (int i = 0; i < 12; ++i) {
    auto *col = keyColumns.add(new KeyColumn(60 + i));
    editView->addAndMakeVisible(col);
  }

  keyboard = std::make_unique<SemiToneKeyboard>();
  keyboard->onKeyPress = [this](int note) { synth.noteOn(1, note, 0.8f); };
  editView->addAndMakeVisible(keyboard.get());

  importButton.onClick = [this] {
    chooser = std::make_unique<juce::FileChooser>(
        "Open Sotero Library...", lastBrowseDirectory, "*.spsa;*.sotero");
    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                             juce::FileBrowserComponent::canSelectFiles,
                         [this](const juce::FileChooser &fc) {
                           auto file = fc.getResult();
                           if (file.existsAsFile()) {
                             loadSoteroLibrary(file);
                           }
                         });
  };
  editView->addAndMakeVisible(importButton);

  exportButton.onClick = [this] {
    updateMetadataFromUI();
    chooser = std::make_unique<juce::FileChooser>(
        "Save Sotero Library...",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile(libraryData.name + ".spsa"),
        "*.spsa;*.sotero");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                             juce::FileBrowserComponent::warnAboutOverwriting,
                         [this](const juce::FileChooser &fc) {
                           auto file = fc.getResult();
                           if (file != juce::File{}) {
                             if (sotero::SoteroArchive::write(
                                     file, libraryData, currentLibraryFile)) {
                               currentLibraryFile = file;
                             }
                           }
                         });
  };
  exportButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colours::orange.darker(0.5f));
  editView->addAndMakeVisible(exportButton);

  // --- PLAYER UI ---
  playerPerformanceView = std::make_unique<PerformanceView>(*this);
  playerSetupView = std::make_unique<SetupView>(*this);

  // --- TABS ---
  addAndMakeVisible(tabs);
  tabs.addTab("EDIT", juce::Colours::darkgrey, editView.get(), false);
  tabs.addTab("PERFORMANCE", juce::Colours::darkgrey,
              playerPerformanceView.get(), false);
  tabs.addTab("SETUP", juce::Colours::darkgrey, playerSetupView.get(), false);

  // Audio Initialization
  formatManager.registerBasicFormats();
  for (int i = 0; i < 16; ++i)
    synth.addVoice(new sotero::SoteroSamplerVoice());

  setAudioChannels(0, 2);
  lastBrowseDirectory =
      juce::File::getSpecialLocation(juce::File::userHomeDirectory);
  setSize(1100, 850); // Increased size to match Player Editor

  // Register as MIDI callback
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  for (auto &device : midiInputs) {
    if (deviceManager.isMidiInputDeviceEnabled(device.identifier))
      deviceManager.addMidiInputDeviceCallback(device.identifier, this);
  }

  updateGridUI();
}

MainComponent::~MainComponent() {
  deviceManager.removeMidiInputDeviceCallback({}, this);
  shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlockExpected;
  spec.numChannels = 2; // Stereo

  synth.setCurrentPlaybackSampleRate(sampleRate);
  midiCollector.reset(sampleRate);

  masterCompressor.prepare(spec);
  masterReverb.prepare(spec);
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  bufferToFill.clearActiveBufferRegion();

  juce::MidiBuffer incomingMidi;
  midiCollector.removeNextBlockOfMessages(incomingMidi,
                                          bufferToFill.numSamples);

  // Process Midi Keyboard State
  keyboardState.processNextMidiBuffer(incomingMidi, 0, bufferToFill.numSamples,
                                      true);

  // Velocity Curve & Midi Tracking
  int curveType = (int)*apvts->getRawParameterValue("velocityCurve");
  juce::MidiBuffer specializedMessages;
  for (const auto metadata : incomingMidi) {
    auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      lastMidiNote.store(msg.getNoteNumber());
      lastMidiVelocity.store(msg.getVelocity());

      float normalizedVel = (float)msg.getVelocity() / 127.0f;
      if (curveType == 0)
        normalizedVel = std::sqrt(normalizedVel);
      else if (curveType == 2)
        normalizedVel = std::pow(normalizedVel, 2.0f);

      int velocity = juce::jlimit(1, 127, (int)(normalizedVel * 127.0f));
      specializedMessages.addEvent(
          juce::MidiMessage::noteOn(msg.getChannel(), msg.getNoteNumber(),
                                    (juce::uint8)velocity),
          metadata.samplePosition);
    } else {
      specializedMessages.addEvent(msg, metadata.samplePosition);
    }
  }

  synth.renderNextBlock(*bufferToFill.buffer, specializedMessages,
                        bufferToFill.startSample, bufferToFill.numSamples);

  // Apply Effects
  auto &buffer = *bufferToFill.buffer;
  int compType = (int)*apvts->getRawParameterValue("masterComp");
  if (compType > 0) {
    masterCompressor.setThreshold(*apvts->getRawParameterValue("compThresh"));
    masterCompressor.setRatio(*apvts->getRawParameterValue("compRatio"));
    masterCompressor.setAttack(*apvts->getRawParameterValue("compAttack"));
    masterCompressor.setRelease(*apvts->getRawParameterValue("compRelease"));

    juce::dsp::AudioBlock<float> block(buffer, bufferToFill.startSample);
    juce::dsp::ProcessContextReplacing<float> context(block);
    masterCompressor.process(context);
  }

  bool revEnabled = (bool)*apvts->getRawParameterValue("revEnable");
  if (revEnabled) {
    reverbParams.roomSize = *apvts->getRawParameterValue("revSize");
    reverbParams.wetLevel = *apvts->getRawParameterValue("revMix");
    reverbParams.dryLevel = 1.0f - (reverbParams.wetLevel * 0.5f);
    masterReverb.setParameters(reverbParams);

    juce::dsp::AudioBlock<float> block(buffer, bufferToFill.startSample);
    juce::dsp::ProcessContextReplacing<float> context(block);
    masterReverb.process(context);
  }

  // Master Gain
  float masterGain = juce::Decibels::decibelsToGain(
      (float)*apvts->getRawParameterValue("masterVol"));
  buffer.applyGain(bufferToFill.startSample, bufferToFill.numSamples,
                   masterGain);

  // Track Levels
  if (bufferToFill.numSamples > 0) {
    lastLevelL.store(buffer.getMagnitude(0, bufferToFill.startSample,
                                         bufferToFill.numSamples));
    lastLevelR.store(buffer.getNumChannels() > 1
                         ? buffer.getMagnitude(1, bufferToFill.startSample,
                                               bufferToFill.numSamples)
                         : lastLevelL.load());
  }
}

void MainComponent::handleIncomingMidiMessage(
    juce::MidiInput *source, const juce::MidiMessage &message) {
  midiCollector.addMessageToQueue(message);
}

void MainComponent::releaseResources() {}

void MainComponent::rebuildSynth() {
  // 1. Prepare all readers and sounds OFF-LOCK
  // We'll collect the sounds first, then swap them into the synth
  juce::ReferenceCountedArray<juce::SynthesiserSound> newSounds;

  for (const auto &m : libraryData.mappings) {
    if (m.samplePath.isNotEmpty()) {
      std::unique_ptr<juce::AudioFormatReader> reader;

      juce::File file(m.samplePath);
      if (file.existsAsFile()) {
        reader.reset(formatManager.createReaderFor(file));
      } else if (currentLibraryFile.existsAsFile()) {
        auto data = sotero::SoteroArchive::extractResource(currentLibraryFile,
                                                           m.samplePath);
        if (data.getSize() > 0) {
          auto stream = std::make_unique<juce::MemoryInputStream>(data, true);
          reader.reset(formatManager.createReaderFor(std::move(stream)));
        }
      }

      if (reader != nullptr) {
        juce::BigInteger range;
        range.setBit(m.midiNote);

        newSounds.add(new sotero::SoteroSamplerSound(
            m.samplePath, *reader, range, m.midiNote, 0.01, 0.1, 10.0,
            m.chokeGroup, m.velocityLow, m.velocityHigh));
      }
    }
  }

  // 2. Quickly swap sounds inside the lock
  const juce::ScopedLock sl(synthLock);
  synth.clearSounds();
  for (auto *s : newSounds)
    synth.addSound(s);
}

void MainComponent::auditionSample(const juce::String &path, int midiNote,
                                   int velocity) {
  const juce::ScopedLock sl(synthLock);
  // Synth note on will trigger the sounds already loaded in rebuildSynth
  synth.noteOn(1, midiNote, (float)velocity / 127.0f);
}

void MainComponent::updateGridUI() {
  for (auto *col : keyColumns) {
    col->clearRegions();
  }

  for (int mIndex = 0; mIndex < libraryData.mappings.size(); ++mIndex) {
    auto &mapping = libraryData.mappings.getReference(mIndex);
    if (mapping.samplePath.isEmpty())
      continue;

    int colIndex = mapping.midiNote - 60;
    if (colIndex >= 0 && colIndex < 12) {
      auto *col = keyColumns[colIndex];
      col->addRegion(mapping);

      auto *region = col->regions.getLast();

      region->onAudition = [this, mIndex](const KeyMapping &m) {
        auditionSample(m.samplePath, m.midiNote,
                       (m.velocityLow + m.velocityHigh) / 2);
      };

      region->onClear = [this, mIndex](const KeyMapping &) {
        // Mark as deleted by clearing path, then update UI and synth
        // We do this asynchronously to avoid deleting the component while we're
        // inside its mouseDown handler.
        juce::MessageManager::callAsync([this, mIndex]() {
          if (mIndex >= 0 && mIndex < libraryData.mappings.size()) {
            libraryData.mappings.getReference(mIndex).samplePath = "";
            updateGridUI();
            rebuildSynth();
          }
        });
      };

      juce::Component::SafePointer<KeyColumn> safeCol(col);
      region->onBoundsChanged = [this, mIndex, safeCol](const KeyMapping &m) {
        if (mIndex >= 0 && mIndex < libraryData.mappings.size()) {
          auto &ref = libraryData.mappings.getReference(mIndex);
          ref.velocityLow = m.velocityLow;
          ref.velocityHigh = m.velocityHigh;
          ref.samplePath = m.samplePath; // Sync path for live eraser support

          if (safeCol != nullptr)
            safeCol->resized(); // Liquid smooth UI update with safety
        }
      };

      region->onDragFinished = [this](const KeyMapping &m) {
        juce::MessageManager::callAsync([this]() {
          updateGridUI(); // Ensure all components reflect the resolved bounds
          rebuildSynth(); // Heavy work only when interaction ends
        });
      };
    }
  }
}

void MainComponent::updateMetadataFromUI() {
  libraryData.name = nameEditor.getText();
  libraryData.author = authorEditor.getText();
}

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff121212));
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRect(getLocalBounds().reduced(5), 1.0f);
}

void MainComponent::loadSoteroLibrary(const juce::File &file) {
  if (!file.existsAsFile())
    return;
  auto imported = sotero::SoteroArchive::readMetadata(file);
  if (imported.mappings.size() > 0) {
    currentLibraryFile = file;
    libraryData = imported;
    nameEditor.setText(libraryData.name);
    authorEditor.setText(libraryData.author);
    updateGridUI();
    rebuildSynth();
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout
MainComponent::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "midiChannel", "Midi Channel", 0, 16, 0));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "velocityCurve", "Velocity Curve", 0, 2, 1));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "masterComp", "Master Compressor Mode", 0, 3, 0));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compThresh", "Threshold", -60.0f, 0.0f, -20.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compRatio", "Ratio", 1.0f, 10.0f, 3.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compAttack", "Attack", 1.0f, 100.0f, 20.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compRelease", "Release", 10.0f, 500.0f, 100.0f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      "revEnable", "Reverb Enable", false));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "revType", "Reverb Type", 0, 3, 0));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "revSize", "Room Size", 0.0f, 1.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "revMix", "Mix", 0.0f, 1.0f, 0.3f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "masterVol", "Master Volume", -60.0f, 6.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "pan0", "Master Pan", -1.0f, 1.0f, 0.0f));
  return {params.begin(), params.end()};
}

void MainComponent::resized() {
  tabs.setBounds(getLocalBounds());

  // Edit View Layout
  auto r = editView->getLocalBounds().reduced(20);
  titleLabel.setBounds(r.removeFromTop(40));
  auto infoArea = r.removeFromTop(40);
  nameEditor.setBounds(infoArea.removeFromLeft(r.getWidth() / 2).reduced(5));
  authorEditor.setBounds(infoArea.reduced(5));
  r.removeFromTop(10);
  auto footerArea = r.removeFromBottom(80);
  auto buttonW = 150;
  exportButton.setBounds(footerArea.removeFromRight(buttonW)
                             .withSizeKeepingCentre(buttonW, 40)
                             .reduced(5));
  importButton.setBounds(footerArea.removeFromRight(buttonW)
                             .withSizeKeepingCentre(buttonW, 40)
                             .reduced(5));
  keyboard->setBounds(r.removeFromBottom(80));
  r.removeFromBottom(5);
  float colWidth = (float)r.getWidth() / 12.0f;
  for (int i = 0; i < 12; ++i) {
    keyColumns[i]->setBounds((int)(r.getX() + i * colWidth), r.getY(),
                             (int)colWidth, r.getHeight());
  }
}

void MainComponent::filesDropped(const juce::StringArray &files, int x, int y) {
  if (files.size() > 0) {
    for (int i = 0; i < keyColumns.size(); ++i) {
      auto *col = keyColumns[i];
      auto colBounds = col->getBounds();

      if (colBounds.contains(x, y)) {
        int localY = y - colBounds.getY();
        float dropVelocityNormalized =
            1.0f - ((float)localY / (float)colBounds.getHeight());
        int targetVelocity =
            juce::jlimit(0, 127, (int)(dropVelocityNormalized * 127.0f));

        int span = 20; // Default span
        int velLow = juce::jlimit(0, 127, targetVelocity - span / 2);
        int velHigh = juce::jlimit(0, 127, targetVelocity + span / 2);

        for (int f = 0; f < files.size(); ++f) {
          // Adjust bounds if they overlap existing regions
          bool overlap = true;
          int attempts = 0;
          while (overlap && attempts < 10) {
            overlap = false;
            for (int mapIdx = 0; mapIdx < libraryData.mappings.size();
                 ++mapIdx) {
              const auto &mCheck = libraryData.mappings.getReference(mapIdx);
              if (mCheck.midiNote == (60 + i) &&
                  mCheck.samplePath.isNotEmpty()) {
                // Check for overlap
                if (velLow <= mCheck.velocityHigh &&
                    velHigh >= mCheck.velocityLow) {
                  overlap = true;

                  // If dropping above an existing region, push up. Otherwise,
                  // push down.
                  if (targetVelocity >=
                      (mCheck.velocityLow + mCheck.velocityHigh) / 2) {
                    velLow = mCheck.velocityHigh + 1;
                    velHigh = velLow + span;
                  } else {
                    velHigh = mCheck.velocityLow - 1;
                    velLow = velHigh - span;
                  }

                  // constrain loop
                  velHigh = juce::jlimit(1, 127, velHigh);
                  velLow = juce::jlimit(0, velHigh - 1, velLow);
                  break;
                }
              }
            }
            attempts++;
          }

          KeyMapping m;
          m.midiNote = 60 + i;
          m.samplePath = files[f];
          m.fileName = juce::File(files[f]).getFileName();
          m.velocityLow = velLow;
          m.velocityHigh = velHigh;
          m.chokeGroup = 0;

          libraryData.mappings.add(m);

          // Shift velocity down for subsequent files if multiple dropped
          velHigh = velLow - 1;
          velLow = juce::jlimit(0, 127, velHigh - span);
        }

        updateGridUI();
        rebuildSynth();

        // Audition the first dropped file
        auditionSample(files[0], 60 + i, targetVelocity);
        return;
      }
    }
  }
}

} // namespace sotero
