#include "MainComponent.h"
#include "../../Common/SoteroArchive.h"
#include "../../SamplerPlayer/Source/SoteroSamplerVoice.h"
#include "BinaryData.h"
#include <thread>

namespace sotero {

// MainComponent implementation
MainComponent::MainComponent()
    : apvts(std::make_unique<juce::AudioProcessorValueTreeState>(
          dummyProcessor, nullptr, "Parameters", createParameterLayout())) {
  libraryData.mappings.clear();

  // --- BUILDERUI Modular Panels ---
  addAndMakeVisible(headerPanel);

  waveform1 =
      std::make_unique<WaveformPanel>(juce::Colours::cyan, "MIC 1 - RESOURCE");
  waveform2 =
      std::make_unique<WaveformPanel>(juce::Colours::red, "MIC 2 - RESOURCE");
  addAndMakeVisible(waveform1.get());
  addAndMakeVisible(waveform2.get());

  addAndMakeVisible(sculptingPanel);
  sculptingPanel.setEnabled(false);
  addAndMakeVisible(mappingPanel);
  addAndMakeVisible(metadataPanel);
  addAndMakeVisible(advancedPanel);

  startTimerHz(30); // Poll for playhead at 30Hz

  // --- Sculpting Panel Wiring ---
  auto updateADSRVisual = [this] {
    if (activeMappingIndex < 0 ||
        activeMappingIndex >= libraryData.mappings.size())
      return;

    auto &m = libraryData.mappings.getReference(activeMappingIndex);
    sculptingPanel.adsrVisualizer.setParams(
        (float)sculptingPanel.attackSlider.getValue() / 5.0f,
        (float)sculptingPanel.decaySlider.getValue() / 5.0f,
        (float)sculptingPanel.sustainSlider.getValue(),
        (float)sculptingPanel.releaseSlider.getValue() / 5.0f,
        m.adsrAttackCurve, m.adsrDecayCurve, m.adsrReleaseCurve,
        1.0f // Peak
    );
  };

  auto &sp = sculptingPanel;

  // Attack
  sp.attackSlider.setRange(0.001, 5.0, 0.001);
  sp.attackSlider.onValueChange = [this, updateADSRVisual] {
    sculptingPanel.attackInput.setText(
        juce::String(sculptingPanel.attackSlider.getValue(), 3),
        juce::dontSendNotification);
    updateADSRVisual();
  };
  sp.attackInput.onReturnKey = [this] {
    sculptingPanel.attackSlider.setValue(
        sculptingPanel.attackInput.getText().getFloatValue(),
        juce::sendNotification);
  };
  sp.adsrVisualizer.onAttackChange = [this](float val) {
    sculptingPanel.attackSlider.setValue(val * 5.0f, juce::sendNotification);
  };

  // Decay
  sp.decaySlider.setRange(0.001, 5.0, 0.001);
  sp.decaySlider.onValueChange = [this, updateADSRVisual] {
    sculptingPanel.decayInput.setText(
        juce::String(sculptingPanel.decaySlider.getValue(), 3),
        juce::dontSendNotification);
    updateADSRVisual();
  };
  sp.decayInput.onReturnKey = [this] {
    sculptingPanel.decaySlider.setValue(
        sculptingPanel.decayInput.getText().getFloatValue(),
        juce::sendNotification);
  };
  sp.adsrVisualizer.onDecayChange = [this](float val) {
    sculptingPanel.decaySlider.setValue(val * 5.0f, juce::sendNotification);
  };

  // Sustain
  sp.sustainSlider.setRange(0.0, 1.0, 0.01);
  sp.sustainSlider.onValueChange = [this, updateADSRVisual] {
    sculptingPanel.sustainInput.setText(
        juce::String(sculptingPanel.sustainSlider.getValue(), 2),
        juce::dontSendNotification);
    updateADSRVisual();
  };
  sp.sustainInput.onReturnKey = [this] {
    sculptingPanel.sustainSlider.setValue(
        sculptingPanel.sustainInput.getText().getFloatValue(),
        juce::sendNotification);
  };
  sp.adsrVisualizer.onSustainChange = [this](float val) {
    sculptingPanel.sustainSlider.setValue(val, juce::sendNotification);
  };

  // Release
  sp.releaseSlider.setRange(0.001, 5.0, 0.001);
  sp.releaseSlider.onValueChange = [this, updateADSRVisual] {
    sculptingPanel.releaseInput.setText(
        juce::String(sculptingPanel.releaseSlider.getValue(), 3),
        juce::dontSendNotification);
    updateADSRVisual();
  };
  sp.releaseInput.onReturnKey = [this] {
    sculptingPanel.releaseSlider.setValue(
        sculptingPanel.releaseInput.getText().getFloatValue(),
        juce::sendNotification);
  };
  sp.adsrVisualizer.onReleaseChange = [this](float val) {
    if (activeMappingIndex >= 0 &&
        activeMappingIndex < libraryData.mappings.size()) {
      sculptingPanel.releaseSlider.setValue(val * 5.0f, juce::sendNotification);
    }
  };
  sp.adsrVisualizer.onAttackCurveChange = [this](float val) {
    if (activeMappingIndex >= 0 &&
        activeMappingIndex < libraryData.mappings.size()) {
      libraryData.mappings.getReference(activeMappingIndex).adsrAttackCurve =
          val;
      rebuildSynth();
    }
  };
  sp.adsrVisualizer.onDecayCurveChange = [this](float val) {
    if (activeMappingIndex >= 0 &&
        activeMappingIndex < libraryData.mappings.size()) {
      libraryData.mappings.getReference(activeMappingIndex).adsrDecayCurve =
          val;
      rebuildSynth();
    }
  };
  sp.adsrVisualizer.onReleaseCurveChange = [this](float val) {
    if (activeMappingIndex >= 0 &&
        activeMappingIndex < libraryData.mappings.size()) {
      libraryData.mappings.getReference(activeMappingIndex).adsrReleaseCurve =
          val;
      rebuildSynth();
    }
  };

  // --- Project Controls Setup ---
  headerPanel.loadBtn.onClick = [this] {
    chooser = std::make_unique<juce::FileChooser>(
        "Select a Sotero Library...",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.sotero");

    auto flags = juce::FileBrowserComponent::openMode |
                 juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(flags, [this](const juce::FileChooser &fc) {
      auto result = fc.getResult();
      if (result.existsAsFile())
        loadSoteroLibrary(result);
    });
  };

  headerPanel.saveBtn.onClick = [this] {
    updateMetadataFromUI();
    chooser = std::make_unique<juce::FileChooser>(
        "Save Sotero Library...",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile(libraryData.name + ".spsa"),
        "*.spsa;*.sotero");

    auto flags = juce::FileBrowserComponent::saveMode |
                 juce::FileBrowserComponent::warnAboutOverwriting;
    chooser->launchAsync(flags, [this](const juce::FileChooser &fc) {
      auto result = fc.getResult();
      if (result != juce::File{})
        sotero::SoteroArchive::write(result, libraryData, currentLibraryFile);
    });
  };

  headerPanel.newBtn.onClick = [this] {
    deselectAllRegions();
    libraryData = LibraryMetadata();
    libraryData.mappings.clear();
    updateGridUI();
  };

  // --- Octave / Mapping Setup ---
  for (int i = -2; i <= 8; ++i) {
    mappingPanel.octaveSelector.addItem("Octave C" + juce::String(i), i + 3);
  }
  mappingPanel.octaveSelector.setSelectedId(mappingPanel.currentOctave + 3);
  mappingPanel.octaveSelector.onChange = [this] {
    updateOctave(mappingPanel.octaveSelector.getSelectedId() - 3);
  };

  // --- Mapping / KeyColumn Callbacks ---
  auto setupCol = [this](KeyColumn *col) {
    col->onFilesDropped = [this, col](const juce::StringArray &files,
                                      int localY) {
      if (files.size() == 0)
        return;

      float dropVelocityNormalized =
          1.0f - ((float)localY / (float)col->getHeight());
      int targetVelocity =
          juce::jlimit(0, 127, (int)(dropVelocityNormalized * 127.0f));

      int span = 20;
      int velLow = juce::jlimit(0, 127, targetVelocity - span / 2);
      int velHigh = juce::jlimit(0, 127, targetVelocity + span / 2);

      for (int i = 0; i < files.size(); ++i) {
        KeyMapping m;
        m.midiNote = col->noteNumber;
        m.micLayer = col->micLayer;
        m.samplePath = files[i];
        m.fileName = juce::File(files[i]).getFileName();
        m.velocityLow = juce::jlimit(0, 126, velLow);
        m.velocityHigh = juce::jlimit(m.velocityLow + 1, 127, velHigh);

        // --- PREVENT STACKING ---
        // Replace existing mappings on this note/layer that overlap this
        // velocity zone
        for (int j = libraryData.mappings.size() - 1; j >= 0; --j) {
          auto &existing = libraryData.mappings.getReference(j);
          if (existing.midiNote == m.midiNote &&
              existing.micLayer == m.micLayer) {
            if (existing.velocityLow <= m.velocityHigh &&
                existing.velocityHigh >= m.velocityLow) {
              existing.samplePath = ""; // Mark for removal in updateGridUI
            }
          }
        }
        libraryData.mappings.add(m);

        velHigh = velLow - 1;
        velLow = juce::jlimit(0, 127, velHigh - span);
      }

      updateGridUI();
      rebuildSynth();
    };
  };

  if (mappingPanel.layer1) {
    for (auto *col : mappingPanel.layer1->columns)
      setupCol(col);
  }
  if (mappingPanel.layer2) {
    for (auto *col : mappingPanel.layer2->columns)
      setupCol(col);
  }

  updateGridUI();

  if (mappingPanel.layer1 && mappingPanel.layer1->keyboard) {
    mappingPanel.layer1->keyboard->onKeyPress = [this](int note) {
      synth.noteOn(1, note + (mappingPanel.currentOctave * 12), 0.8f);
    };
  }
  if (mappingPanel.layer2 && mappingPanel.layer2->keyboard) {
    mappingPanel.layer2->keyboard->onKeyPress = [this](int note) {
      synth.noteOn(1, note + (mappingPanel.currentOctave * 12), 0.8f);
    };
  }

  // --- Audio Setup ---
  formatManager.registerBasicFormats();
  for (int i = 0; i < 16; ++i)
    synth.addVoice(new sotero::SoteroSamplerVoice());

  // --- TO PLAYER Toggles Wiring (Independent Layers) ---
  auto updateLayerBypass = [this](int layerIdx) {
    bool isEnabled = (layerIdx == 0)
                         ? mappingPanel.layer1->toPlayerToggle.getToggleState()
                         : mappingPanel.layer2->toPlayerToggle.getToggleState();

    // Header toggle shows an "overall" state (e.g., ON if ANY is ON, or just
    // syncs if preferred) User requested independent clicking, so we just
    // trigger rebuild.
    rebuildSynth();
  };

  mappingPanel.layer1->toPlayerToggle.onStateChange =
      [this, updateLayerBypass] { updateLayerBypass(0); };
  mappingPanel.layer2->toPlayerToggle.onStateChange =
      [this, updateLayerBypass] { updateLayerBypass(1); };

  // Sync Global Header toggle to Layer 1 for now, or make it a "Master" toggle
  // that overrides
  headerPanel.toPlayerToggle.onStateChange = [this] {
    bool state = headerPanel.toPlayerToggle.getToggleState();
    mappingPanel.layer1->toPlayerToggle.setToggleState(state,
                                                       juce::sendNotification);
    mappingPanel.layer2->toPlayerToggle.setToggleState(state,
                                                       juce::sendNotification);
  };

  setAudioChannels(0, 2);
  lastBrowseDirectory =
      juce::File::getSpecialLocation(juce::File::userHomeDirectory);
  setSize(1100, 850);

  mappingPanel.layerSyncLock.onStateChange = [this] {
    if (mappingPanel.layerSyncLock.getToggleState()) {
      alignLayers();
    }
  };

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
  masterToneFilter.prepare(spec);

  for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto *v = dynamic_cast<sotero::SoteroSamplerVoice *>(synth.getVoice(i)))
      v->prepare(spec);
  }
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
        normalizedVel = normalizedVel * normalizedVel;

      int velocity = juce::jlimit(1, 127, (int)(normalizedVel * 127.0f));
      specializedMessages.addEvent(
          juce::MidiMessage::noteOn(msg.getChannel(), msg.getNoteNumber(),
                                    (juce::uint8)velocity),
          metadata.samplePosition);
    } else {
      specializedMessages.addEvent(msg, metadata.samplePosition);
    }
  }

  // --- Master Parameters ---
  float masterPitch = *apvts->getRawParameterValue("masterPitch");
  for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto *v = dynamic_cast<sotero::SoteroSamplerVoice *>(synth.getVoice(i)))
      v->setMasterPitch(masterPitch);
  }

  // --- Audio Processing ---
  synth.renderNextBlock(*bufferToFill.buffer, specializedMessages,
                        bufferToFill.startSample, bufferToFill.numSamples);

  // Apply Effects
  auto &buffer = *bufferToFill.buffer;

  // --- EFFECT BYPASS (Pure Audition) ---
  if (!headerPanel.toPlayerToggle.getToggleState()) {
    // Apply ONLY basic Master Gain
    float masterGain = juce::Decibels::decibelsToGain(
        (float)*apvts->getRawParameterValue("masterVol"));
    buffer.applyGain(bufferToFill.startSample, bufferToFill.numSamples,
                     masterGain);

    if (bufferToFill.numSamples > 0) {
      lastLevelL.store(buffer.getMagnitude(0, bufferToFill.startSample,
                                           bufferToFill.numSamples));
      lastLevelR.store(buffer.getNumChannels() > 1
                           ? buffer.getMagnitude(1, bufferToFill.startSample,
                                                 bufferToFill.numSamples)
                           : lastLevelL.load());
    }
    return;
  }

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

  // Master Tone Filter
  float masterTone = *apvts->getRawParameterValue("masterTone");
  if (std::abs(masterTone) > 0.05f) {
    if (masterTone < 0) {
      masterToneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
      float freq =
          juce::jmap(std::abs(masterTone), 0.0f, 1.0f, 20000.0f, 400.0f);
      masterToneFilter.setCutoffFrequency(freq);
    } else {
      masterToneFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
      float freq = juce::jmap(masterTone, 0.0f, 1.0f, 20.0f, 3000.0f);
      masterToneFilter.setCutoffFrequency(freq);
    }
    juce::dsp::AudioBlock<float> block(buffer, bufferToFill.startSample);
    juce::dsp::ProcessContextReplacing<float> context(block);
    masterToneFilter.process(context);
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
  // 1. Prepare all readers and sounds OFF-LOCK in a background thread
  // We use a lambda to do the heavy lifting
  auto buildTask = [this]() {
    juce::ReferenceCountedArray<juce::SynthesiserSound> newSounds;

    std::unique_ptr<juce::FileInputStream> stream;
    if (currentLibraryFile.existsAsFile())
      stream = std::make_unique<juce::FileInputStream>(currentLibraryFile);

    for (const auto &m : libraryData.mappings) {
      if (m.samplePath.isNotEmpty()) {
        std::unique_ptr<juce::AudioFormatReader> reader;

        juce::File file(m.samplePath);
        if (file.existsAsFile()) {
          reader.reset(formatManager.createReaderFor(file));
        } else if (stream != nullptr && stream->openedOk()) {
          auto data =
              sotero::SoteroArchive::extractResource(*stream, m.samplePath);
          if (data.getSize() > 0) {
            auto memStream =
                std::make_unique<juce::MemoryInputStream>(data, true);
            reader.reset(formatManager.createReaderFor(std::move(memStream)));
          }
        }

        if (reader != nullptr) {
          juce::BigInteger range;
          range.setBit(m.midiNote);

          bool bypassLayer1 =
              !mappingPanel.layer1->toPlayerToggle.getToggleState();
          bool bypassLayer2 =
              !mappingPanel.layer2->toPlayerToggle.getToggleState();
          bool isBypassed = (m.micLayer == 0) ? bypassLayer1 : bypassLayer2;

          newSounds.add(new sotero::SoteroSamplerSound(
              m.samplePath, *reader, range, m.midiNote, 0.01, 0.1, 10.0,
              m.chokeGroup, m.velocityLow, m.velocityHigh, m.sampleStart,
              m.sampleEnd, m.fadeIn, m.fadeOut, m.volumeMultiplier,
              m.fineTuneCents, m.micLayer, m.adsrAttack, m.adsrDecay,
              m.adsrSustain, m.adsrRelease, m.adsrAttackCurve, m.adsrDecayCurve,
              m.adsrReleaseCurve, m.filterType, m.filterCutoff,
              m.filterResonance, libraryData.enableADSR && !isBypassed,
              libraryData.enableFilter && !isBypassed));
        }
      }
    }

    // 2. Quickly swap sounds inside the lock on the Message Thread
    juce::MessageManager::callAsync([this, newSounds]() {
      const juce::ScopedLock sl(synthLock);
      synth.clearSounds();
      for (auto *s : newSounds)
        synth.addSound(s);
    });
  };

  // Launch the task on a background thread
  std::thread(buildTask).detach();
}

void MainComponent::auditionSample(const juce::String &path, int midiNote,
                                   int velocity) {
  const juce::ScopedLock sl(synthLock);
  // Synth note on will trigger the sounds already loaded in rebuildSynth
  synth.noteOn(1, midiNote, (float)velocity / 127.0f);
}

void MainComponent::auditionSampleOff(int midiNote) {
  const juce::ScopedLock sl(synthLock);
  // Synth note off ensures the ADSR enters release phase and frees polyphony
  synth.noteOff(1, midiNote, 0.0f, true);
}

void MainComponent::updateGridUI() {
  if (mappingPanel.layer1) {
    for (auto *col : mappingPanel.layer1->columns)
      col->clearRegions();
  }
  if (mappingPanel.layer2) {
    for (auto *col : mappingPanel.layer2->columns)
      col->clearRegions();
  }

  for (int mIndex = 0; mIndex < libraryData.mappings.size(); ++mIndex) {
    auto &mapping = libraryData.mappings.getReference(mIndex);
    if (mapping.samplePath.isEmpty())
      continue;

    int baseNote = (mappingPanel.currentOctave + 3) * 12;
    int colIndex = mapping.midiNote - baseNote;

    if (colIndex >= 0 && colIndex < 12) {
      KeyColumn *col = nullptr;
      if (mapping.micLayer == 0 && mappingPanel.layer1)
        col = mappingPanel.layer1->columns[colIndex];
      else if (mapping.micLayer == 1 && mappingPanel.layer2)
        col = mappingPanel.layer2->columns[colIndex];

      if (col == nullptr)
        continue;

      col->addRegion(mapping);
      auto *region = col->regions.getLast();

      region->onAudition = [this](const KeyMapping &m) {
        auditionSample(m.samplePath, m.midiNote,
                       (m.velocityLow + m.velocityHigh) / 2);
      };
      region->onAuditionEnd = [this](const KeyMapping &m) {
        auditionSampleOff(m.midiNote);
      };

      if (mIndex == activeMappingIndex)
        region->setActive(true);

      region->onSelect = [this, mIndex]() {
        // Deselect others visually without destroying everything
        activeMappingIndex = mIndex;
        for (auto *l : {mappingPanel.layer1.get(), mappingPanel.layer2.get()})
          if (l)
            for (auto *c : l->columns)
              for (auto *r : c->regions)
                r->setActive(false);

        sculptingPanel.setEnabled(true);
        auto &m = libraryData.mappings.getReference(mIndex);

        sculptingPanel.attackSlider.setValue(m.adsrAttack,
                                             juce::dontSendNotification);
        sculptingPanel.decaySlider.setValue(m.adsrDecay,
                                            juce::dontSendNotification);
        sculptingPanel.sustainSlider.setValue(m.adsrSustain,
                                              juce::dontSendNotification);
        sculptingPanel.releaseSlider.setValue(m.adsrRelease,
                                              juce::dontSendNotification);

        sculptingPanel.adsrVisualizer.setParams(
            m.adsrAttack / 5.0f, m.adsrDecay / 5.0f, m.adsrSustain,
            m.adsrRelease / 5.0f, m.adsrAttackCurve, m.adsrDecayCurve,
            m.adsrReleaseCurve, 1.0f);

        sculptingPanel.filterTypeSelector.setSelectedId(
            m.filterType + 1, juce::dontSendNotification);
        sculptingPanel.cutoffSlider.setValue(m.filterCutoff,
                                             juce::dontSendNotification);
        sculptingPanel.resSlider.setValue(m.filterResonance,
                                          juce::dontSendNotification);

        auto updateData = [this, mIndex]() {
          if (mIndex >= 0 && mIndex < libraryData.mappings.size()) {
            auto &ref = libraryData.mappings.getReference(mIndex);
            ref.adsrAttack = (float)sculptingPanel.attackSlider.getValue();
            ref.adsrDecay = (float)sculptingPanel.decaySlider.getValue();
            ref.adsrSustain = (float)sculptingPanel.sustainSlider.getValue();
            ref.adsrRelease = (float)sculptingPanel.releaseSlider.getValue();
            ref.filterType =
                sculptingPanel.filterTypeSelector.getSelectedId() - 1;
            ref.filterCutoff = (float)sculptingPanel.cutoffSlider.getValue();
            ref.filterResonance = (float)sculptingPanel.resSlider.getValue();
            rebuildSynth();
          }
        };

        sculptingPanel.attackSlider.onValueChange = updateData;
        sculptingPanel.decaySlider.onValueChange = updateData;
        sculptingPanel.sustainSlider.onValueChange = updateData;
        sculptingPanel.releaseSlider.onValueChange = updateData;
        sculptingPanel.filterTypeSelector.onChange = updateData;
        sculptingPanel.cutoffSlider.onValueChange = updateData;
        sculptingPanel.resSlider.onValueChange = updateData;
        updateData(); // Apply initial values and rebuild synth
        // Do NOT call updateGridUI() here as it destroys the very sliders we
        // are using! Instead, just repaint the regions to show the active state
        // if needed
        for (auto *l : {mappingPanel.layer1.get(), mappingPanel.layer2.get()})
          if (l)
            for (auto *c : l->columns)
              for (auto *r : c->regions)
                r->repaint();
      };

      region->onErase = [this, mIndex]() {
        if (mIndex >= 0 && mIndex < libraryData.mappings.size()) {
          auto &orig = libraryData.mappings.getReference(mIndex);
          int oldNote = orig.midiNote;
          int oldLo = orig.velocityLow;
          int oldHi = orig.velocityHigh;
          orig.samplePath = "";

          if (mappingPanel.layerSyncLock.getToggleState()) {
            int otherLayer = 1 - orig.micLayer;
            for (int j = 0; j < libraryData.mappings.size(); ++j) {
              auto &other = libraryData.mappings.getReference(j);
              if (other.micLayer == otherLayer && other.midiNote == oldNote &&
                  other.velocityLow == oldLo && other.velocityHigh == oldHi &&
                  other.samplePath.isNotEmpty()) {
                other.samplePath = "";
                break;
              }
            }
          }
        }
      };

      region->onDragStart = [this](bool top, bool bottom) {
        dragStickyTop = top;
        dragStickyBottom = bottom;
      };

      region->onClear = [this, mIndex](const KeyMapping &) {
        if (mIndex >= 0 && mIndex < libraryData.mappings.size()) {
          libraryData.mappings.getReference(mIndex).samplePath = "";
          // Async call to avoid destroying 'this' inside its own event handler
          juce::MessageManager::callAsync([this] {
            updateGridUI();
            rebuildSynth();
          });
        }
      };

      juce::Component::SafePointer<KeyColumn> safeCol(col);
      region->onBoundsChanged = [this, mIndex, safeCol,
                                 region](const KeyMapping &m) {
        if (mIndex >= 0 && mIndex < libraryData.mappings.size()) {
          auto &ref = libraryData.mappings.getReference(mIndex);
          int targetLo = m.velocityLow;
          int targetHi = m.velocityHigh;

          bool syncActive = mappingPanel.layerSyncLock.getToggleState();
          int otherIndex = syncActive ? findCounterpart(mIndex) : -1;

          // 1. Resolve collisions in primary layer (with push-back)
          resolveCollisions(ref.midiNote, ref.micLayer, targetLo, targetHi,
                            mIndex, true, true);

          // 2. Resolve collisions in other layer if sync is on
          if (syncActive && otherIndex != -1) {
            auto &other = libraryData.mappings.getReference(otherIndex);
            other.velocityLow = targetLo;
            other.velocityHigh = targetHi;
            // Also pushes back if other hits bound
            resolveCollisions(other.midiNote, other.micLayer, other.velocityLow,
                              other.velocityHigh, otherIndex, true, false);
            // Correct the primary mapping too if other pushed back
            targetLo = other.velocityLow;
            targetHi = other.velocityHigh;
          }

          // 3. Apply changes back to data
          ref.velocityLow = targetLo;
          ref.velocityHigh = targetHi;

          // 4. Update the actual UI components from the new data
          // IMPORTANT: Even if 'region' is dragging, we want to update its
          // internal 'currentMapping' to reflect any push-back/capping
          // that resolveCollisions performed.
          region->updateFromMapping(ref);

          updateColumnRegions(ref.midiNote, ref.micLayer);
          if (syncActive && otherIndex != -1) {
            auto &other = libraryData.mappings.getReference(otherIndex);
            updateColumnRegions(other.midiNote, other.micLayer);
          }
        }
      };

      region->onRequestMove = [this, mIndex](int deltaNote) {
        if (mIndex >= 0 && mIndex < libraryData.mappings.size()) {
          auto &ref = libraryData.mappings.getReference(mIndex);
          int oldNote = ref.midiNote;
          int oldLo = ref.velocityLow;
          int oldHi = ref.velocityHigh;
          int targetNote = juce::jlimit(0, 127, oldNote + deltaNote);

          if (targetNote == oldNote)
            return;

          bool syncActive = mappingPanel.layerSyncLock.getToggleState();
          int otherIndex = syncActive ? findCounterpart(mIndex) : -1;

          // 1. Resolve collisions in primary layer at destination
          resolveCollisions(targetNote, ref.micLayer, oldLo, oldHi, mIndex,
                            true, true);

          // 2. If sync is on, resolve in other layer at destination
          if (syncActive && otherIndex != -1) {
            auto &other = libraryData.mappings.getReference(otherIndex);
            other.midiNote = targetNote;
            resolveCollisions(targetNote, other.micLayer, oldLo, oldHi,
                              otherIndex, true, false);
          }

          // 3. Apply changes
          ref.midiNote = targetNote;
          if (syncActive && otherIndex != -1) {
            libraryData.mappings.getReference(otherIndex).midiNote = targetNote;
          }

          juce::MessageManager::callAsync([this] {
            updateGridUI();
            rebuildSynth();
          });
        }
      };

      region->onDragFinished = [this](const KeyMapping &) {
        dragStickyTop = false;
        dragStickyBottom = false;
        juce::MessageManager::callAsync([this] {
          updateGridUI();
          rebuildSynth();
        });
      };
    }
  }
}

void MainComponent::timerCallback() {
  const juce::ScopedLock sl(synthLock);
  float latestTime = -1.0f;

  for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto *v = dynamic_cast<SoteroSamplerVoice *>(synth.getVoice(i))) {
      if (v->isVoiceActive()) {
        latestTime = v->getADSRProgress(); // I need to add this method to Voice
      }
    }
  }

  sculptingPanel.adsrVisualizer.setPlayheadTime(latestTime);
}

void MainComponent::updateMetadataFromUI() {
  libraryData.name = metadataPanel.nameEditor.getText();
  libraryData.author = metadataPanel.authorEditor.getText();
}

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff121212));
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRect(getLocalBounds().reduced(5), 1.0f);
}

void MainComponent::deselectAllRegions() {
  activeMappingIndex = -1;
  sculptingPanel.setEnabled(false);

  // Reset ADSR visualizer and sliders to default
  sculptingPanel.attackSlider.setValue(0.1, juce::dontSendNotification);
  sculptingPanel.decaySlider.setValue(0.1, juce::dontSendNotification);
  sculptingPanel.sustainSlider.setValue(1.0, juce::dontSendNotification);
  sculptingPanel.releaseSlider.setValue(0.2, juce::dontSendNotification);
  sculptingPanel.adsrVisualizer.setParams(0.1f / 5.0f, 0.1f / 5.0f, 1.0f,
                                          0.2f / 5.0f, 0.0f, 0.0f, 0.0f, 1.0f);

  if (mappingPanel.layer1) {
    for (auto *col : mappingPanel.layer1->columns) {
      for (auto *region : col->regions)
        region->setActive(false);
    }
  }
  if (mappingPanel.layer2) {
    for (auto *col : mappingPanel.layer2->columns) {
      for (auto *region : col->regions)
        region->setActive(false);
    }
  }
}

void MainComponent::updateOctave(int newOctave) {
  mappingPanel.updateOctave(newOctave);
  updateGridUI();
  if (mappingPanel.layer1 && mappingPanel.layer1->keyboard)
    mappingPanel.layer1->keyboard->repaint();
  if (mappingPanel.layer2 && mappingPanel.layer2->keyboard)
    mappingPanel.layer2->keyboard->repaint();
}

void MainComponent::loadSoteroLibrary(const juce::File &file) {
  if (!file.existsAsFile())
    return;
  auto imported = sotero::SoteroArchive::readMetadata(file);
  if (imported.mappings.size() > 0) {
    currentLibraryFile = file;
    libraryData = imported;
    metadataPanel.nameEditor.setText(libraryData.name);
    metadataPanel.authorEditor.setText(libraryData.author);

    // Load artwork if present
    currentArtwork = juce::Image(); // Reset
    if (libraryData.artworkPath.isNotEmpty()) {
      auto artData =
          sotero::SoteroArchive::extractResource(file, libraryData.artworkPath);
      if (artData.getSize() > 0) {
        currentArtwork = juce::ImageFileFormat::loadFrom(artData.getData(),
                                                         artData.getSize());
      }
    }

    // Updated to use modular sculptingPanel
    // (Note: libraryData.enableADSR etc. will be mapped to UI in Phase 7)
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
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "masterPitch", "Master Pitch", -12, 12, 0));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "masterTone", "Master Tone", -1.0f, 1.0f, 0.0f));
  return {params.begin(), params.end()};
}

void MainComponent::resized() {
  auto r = getLocalBounds();
  headerPanel.setBounds(r.removeFromTop(70));

  // Top Area (Waveforms + ADSR)
  auto topArea = r.removeFromTop(220);
  int waveformW = topArea.getWidth() / 4;

  if (waveform1)
    waveform1->setBounds(topArea.removeFromLeft(waveformW).reduced(2));
  if (waveform2)
    waveform2->setBounds(topArea.removeFromLeft(waveformW).reduced(2));
  sculptingPanel.setBounds(topArea.reduced(2));

  // Bottom Area (Mapping + Workspace)
  auto bottomArea = r;
  mappingPanel.setBounds(
      bottomArea.removeFromLeft(bottomArea.getWidth() * 0.65f).reduced(2));

  auto workspaceArea = bottomArea;
  advancedPanel.setBounds(
      workspaceArea.removeFromTop(workspaceArea.getHeight() * 0.45f)
          .reduced(2));
  metadataPanel.setBounds(workspaceArea.reduced(2));
}

MainComponent::MetadataPanel::MetadataPanel() {
  addAndMakeVisible(nameEditor);
  addAndMakeVisible(authorEditor);
  addAndMakeVisible(dateEditor);
  addAndMakeVisible(infoEditor);
  addAndMakeVisible(nameLabel);
  addAndMakeVisible(authorLabel);
  addAndMakeVisible(dateLabel);
  addAndMakeVisible(infoLabel);
  addAndMakeVisible(artworkDrop);
  addAndMakeVisible(artworkLabel);

  nameLabel.setText("LIBRARY NAME", juce::dontSendNotification);
  nameLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
  authorLabel.setText("AUTHOR", juce::dontSendNotification);
  authorLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);

  artworkLabel.setJustificationType(juce::Justification::centred);
  artworkLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
}

void MainComponent::MetadataPanel::resized() {
  auto r = getLocalBounds().reduced(5);

  auto artworkArea = r.removeFromTop(120);
  auto artSquare = artworkArea.removeFromLeft(120);
  artworkDrop.setBounds(artSquare);
  artworkLabel.setBounds(artSquare);

  auto fields = artworkArea.reduced(10, 0);
  nameLabel.setBounds(fields.removeFromTop(20));
  nameEditor.setBounds(fields.removeFromTop(25));
  fields.removeFromTop(5);
  authorLabel.setBounds(fields.removeFromTop(20));
  authorEditor.setBounds(fields.removeFromTop(30));
}

void MainComponent::MetadataPanel::paint(juce::Graphics &g) {
  g.setColour(juce::Colours::black.withAlpha(0.2f));
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);
}

void MainComponent::WaveformPanel::paint(juce::Graphics &g) {
  auto r = getLocalBounds().reduced(2).toFloat();
  g.setColour(bgColor.withAlpha(0.1f));
  g.fillRoundedRectangle(r, 4.0f);
  g.setColour(bgColor.withAlpha(0.3f));
  g.drawRoundedRectangle(r, 4.0f, 1.0f);

  g.setColour(juce::Colours::white.withAlpha(0.6f));
  g.setFont(juce::Font(14.0f, juce::Font::bold));
  g.drawText(title, getLocalBounds().removeFromTop(25).reduced(10, 0),
             juce::Justification::left);

  // Waveform placeholder logic
  g.setColour(bgColor.withAlpha(0.5f));
  auto waveR = getLocalBounds().reduced(10, 30);
  g.drawHorizontalLine(waveR.getCentreY(), (float)waveR.getX(),
                       (float)waveR.getRight());
}

void MainComponent::SculptingPanel::paint(juce::Graphics &g) {
  auto r = getLocalBounds().reduced(5);
  g.setColour(juce::Colour(0xff0a0a0a));
  g.fillRoundedRectangle(r.toFloat(), 4.0f);
}

void MainComponent::WaveformPanel::resized() {
  toPlayerToggle.setBounds(getWidth() - 100, 5, 90, 20);
}

void MainComponent::SculptingPanel::resized() {
  auto r = getLocalBounds().reduced(5);
  auto graphArea = r.removeFromTop(120); // ADSR visualizer area
  adsrVisualizer.setBounds(graphArea.reduced(5));

  auto slidersArea = r;
  float w = slidersArea.getWidth() / 4.0f;

  auto row1 = slidersArea.removeFromTop(70);
  auto aArea = row1.removeFromLeft(w).reduced(2);
  attackSlider.setBounds(aArea.removeFromTop(aArea.getHeight() - 20));
  attackInput.setBounds(aArea);

  auto dArea = row1.removeFromLeft(w).reduced(2);
  decaySlider.setBounds(dArea.removeFromTop(dArea.getHeight() - 20));
  decayInput.setBounds(dArea);

  auto sArea = row1.removeFromLeft(w).reduced(2);
  sustainSlider.setBounds(sArea.removeFromTop(sArea.getHeight() - 20));
  sustainInput.setBounds(sArea);

  auto rArea = row1.reduced(2);
  releaseSlider.setBounds(rArea.removeFromTop(rArea.getHeight() - 20));
  releaseInput.setBounds(rArea);

  auto row2 = slidersArea;
  filterTypeSelector.setBounds(row2.removeFromLeft(w).reduced(2));
  cutoffSlider.setBounds(row2.removeFromLeft(w).reduced(2));
  resSlider.setBounds(row2.removeFromLeft(w).reduced(2));
  velSensSlider.setBounds(row2.reduced(2));
}

void MainComponent::MappingPanel::resized() {
  auto r = getLocalBounds().reduced(5);

  // Bar at the bottom for controls
  auto ctrlBar = r.removeFromBottom(25);
  octaveSelector.setBounds(ctrlBar.removeFromLeft(125).reduced(2));
  layerSyncLock.setBounds(ctrlBar.removeFromLeft(125).reduced(2));

  int gap = 24;
  auto layerArea = r;
  int w = (layerArea.getWidth() - gap) / 2;

  if (layer1)
    layer1->setBounds(layerArea.removeFromLeft(w).reduced(2));

  layerArea.removeFromLeft(gap);

  if (layer2)
    layer2->setBounds(layerArea.reduced(2));
}

void MainComponent::MappingPanel::updateOctave(int newOctave) {
  currentOctave = newOctave;
  int baseNote = (currentOctave + 3) * 12; // C3 is octave 0 internally (60)
  if (layer1) {
    for (int i = 0; i < 12; ++i)
      layer1->columns[i]->noteNumber = baseNote + i;
  }
  if (layer2) {
    for (int i = 0; i < 12; ++i)
      layer2->columns[i]->noteNumber = baseNote + i;
  }
}

void MainComponent::alignLayers() {
  for (int note = 0; note < 128; ++note) {
    juce::Array<int> l1Indices, l2Indices;
    for (int i = 0; i < libraryData.mappings.size(); ++i) {
      auto &m = libraryData.mappings.getReference(i);
      if (m.samplePath.isEmpty() || m.midiNote != note)
        continue;
      if (m.micLayer == 0)
        l1Indices.add(i);
      else if (m.micLayer == 1)
        l2Indices.add(i);
    }

    // Sort by velocityLow
    auto sortByVel = [this](int a, int b) {
      return libraryData.mappings.getReference(a).velocityLow <
             libraryData.mappings.getReference(b).velocityLow;
    };
    std::sort(l1Indices.begin(), l1Indices.end(), sortByVel);
    std::sort(l2Indices.begin(), l2Indices.end(), sortByVel);

    // Link 1-to-1 by rank
    // Link 1-to-1 by rank
    int count = juce::jmin(l1Indices.size(), l2Indices.size());
    for (int k = 0; k < count; ++k) {
      auto &m1 = libraryData.mappings.getReference(l1Indices[k]);
      auto &m2 = libraryData.mappings.getReference(l2Indices[k]);

      int targetLo = m1.velocityLow;
      int targetHi = m1.velocityHigh;

      // Resolve collisions in Layer 2 for the target range before snapping
      // Use allowCrossSync = false here because we are already in a batch sync
      // operation
      resolveCollisions(note, 1, targetLo, targetHi, l2Indices[k], false);

      m2.velocityLow = targetLo;
      m2.velocityHigh = targetHi;
    }
  }
  updateGridUI();
  rebuildSynth();
}

int MainComponent::findCounterpart(int mIndex) {
  if (mIndex < 0 || mIndex >= libraryData.mappings.size())
    return -1;
  auto &src = libraryData.mappings.getReference(mIndex);

  juce::Array<int> srcIndices, dstIndices;
  for (int i = 0; i < libraryData.mappings.size(); ++i) {
    auto &m = libraryData.mappings.getReference(i);
    if (m.samplePath.isEmpty() || m.midiNote != src.midiNote)
      continue;
    if (m.micLayer == src.micLayer)
      srcIndices.add(i);
    else
      dstIndices.add(i);
  }

  // Sort both by velocity
  auto sortByVel = [this](int a, int b) {
    return libraryData.mappings.getReference(a).velocityLow <
           libraryData.mappings.getReference(b).velocityLow;
  };
  std::sort(srcIndices.begin(), srcIndices.end(), sortByVel);
  std::sort(dstIndices.begin(), dstIndices.end(), sortByVel);

  int rank = srcIndices.indexOf(mIndex);
  if (rank >= 0 && rank < dstIndices.size()) {
    return dstIndices[rank];
  }
  return -1;
}

bool MainComponent::isRangeFree(int note, int micLayer, int lo, int hi,
                                int excludeIndex) {
  for (int i = 0; i < libraryData.mappings.size(); ++i) {
    if (i == excludeIndex)
      continue;
    const auto &m = libraryData.mappings.getReference(i);
    if (m.samplePath.isEmpty())
      continue;
    if (m.midiNote == note && m.micLayer == micLayer) {
      if (!(hi < m.velocityLow || lo > m.velocityHigh))
        return false;
    }
  }
  return true;
}

void MainComponent::resolveCollisions(int note, int micLayer, int &targetLo,
                                      int &targetHi, int excludeIndex,
                                      bool allowCrossSync,
                                      bool isPrimaryTarget) {
  static int depth = 0;
  if (++depth > 128) {
    --depth;
    return;
  }

  bool syncActive =
      allowCrossSync && mappingPanel.layerSyncLock.getToggleState();

  // 1. Identify and categorize neighbors
  struct Neighbor {
    int index;
    int lo, hi;
    bool operator==(const Neighbor &other) const {
      return index == other.index;
    }
  };
  juce::Array<Neighbor> above, below;

  for (int i = 0; i < libraryData.mappings.size(); ++i) {
    if (i == excludeIndex)
      continue;
    auto &m = libraryData.mappings.getReference(i);
    if (m.samplePath.isEmpty() || m.midiNote != note || m.micLayer != micLayer)
      continue;

    // Strict positional categorization
    if (m.velocityLow >= targetHi ||
        (m.velocityLow > targetLo && m.velocityHigh > targetHi))
      above.add({i, m.velocityLow, m.velocityHigh});
    else
      below.add({i, m.velocityLow, m.velocityHigh});
  }

  // 2. Sort outward from target
  std::sort(above.begin(), above.end(),
            [](const Neighbor &a, const Neighbor &b) { return a.lo < b.lo; });
  std::sort(below.begin(), below.end(),
            [](const Neighbor &a, const Neighbor &b) { return a.hi > b.hi; });

  auto syncNeighbor = [this, note, micLayer, syncActive](int idx) {
    if (syncActive) {
      int otherIdx = findCounterpart(idx);
      if (otherIdx != -1) {
        auto &orig = libraryData.mappings.getReference(idx);
        auto &other = libraryData.mappings.getReference(otherIdx);
        other.velocityLow = orig.velocityLow;
        other.velocityHigh = orig.velocityHigh;
        resolveCollisions(note, 1 - micLayer, other.velocityLow,
                          other.velocityHigh, otherIdx, true, false);
      }
    }
  };

  // 3. Resolve Above (Stick/Glue then Push)
  for (auto &c : above) {
    auto &m = libraryData.mappings.getReference(c.index);

    // ADHESIVE (PULL): If we were glued at start
    bool shouldPull =
        isPrimaryTarget && dragStickyTop && (above.indexOf(c) == 0);

    if (targetHi >= m.velocityLow || shouldPull) {
      int oldLo = m.velocityLow;
      int oldHi = m.velocityHigh;

      // GLUE LOGIC: Move the neighbor's bottom edge (resize it)
      int newMLo = targetHi + 1;
      int newMHi = oldHi; // Default: Keep top edge (Resize)

      // If neighbor would be squashed, it MUST push
      if (newMHi <= newMLo) {
        int mRange = juce::jmax(1, oldHi - oldLo);
        newMHi = juce::jlimit(0, 127, newMLo + mRange);
        if (newMHi == 127)
          newMLo = 127 - mRange;
      }

      resolveCollisions(note, micLayer, newMLo, newMHi, c.index, allowCrossSync,
                        false);
      m.velocityLow = newMLo;
      m.velocityHigh = newMHi;

      if (m.velocityLow <= targetHi) {
        targetHi = juce::jmax(0, m.velocityLow - 1);
        if (targetLo > targetHi)
          targetLo = targetHi;
      }

      if (m.velocityLow != oldLo || m.velocityHigh != oldHi)
        syncNeighbor(c.index);
    }
  }

  // 4. Resolve Below (Stick/Glue then Push)
  for (auto &c : below) {
    auto &m = libraryData.mappings.getReference(c.index);

    // ADHESIVE (PULL): If we were glued at start
    bool shouldPull =
        isPrimaryTarget && dragStickyBottom && (below.indexOf(c) == 0);

    if (targetLo <= m.velocityHigh || shouldPull) {
      int oldLo = m.velocityLow;
      int oldHi = m.velocityHigh;

      // GLUE LOGIC: Move the neighbor's top edge (resize it)
      int newMHi = targetLo - 1;
      int newMLo = oldLo; // Default: Keep bottom edge (Resize)

      // If neighbor would be squashed, it MUST push
      if (newMLo >= newMHi) {
        int mRange = juce::jmax(1, oldHi - oldLo);
        newMLo = juce::jlimit(0, 127, newMHi - mRange);
        if (newMLo == 0)
          newMHi = mRange;
      }

      resolveCollisions(note, micLayer, newMLo, newMHi, c.index, allowCrossSync,
                        false);
      m.velocityLow = newMLo;
      m.velocityHigh = newMHi;

      if (m.velocityHigh >= targetLo) {
        targetLo = juce::jmin(127, m.velocityHigh + 1);
        if (targetHi < targetLo)
          targetHi = targetLo;
      }

      if (m.velocityLow != oldLo || m.velocityHigh != oldHi)
        syncNeighbor(c.index);
    }
  }

  --depth;
}

void MainComponent::updateColumnRegions(int note, int layer) {
  MappingPanel::LayerView *targetLayer =
      (layer == 0) ? mappingPanel.layer1.get() : mappingPanel.layer2.get();
  if (!targetLayer)
    return;

  int baseNote = (mappingPanel.currentOctave + 3) * 12;
  int colIndex = note - baseNote;
  if (colIndex < 0 || colIndex >= 12)
    return;

  KeyColumn *col = targetLayer->columns[colIndex];
  if (!col)
    return;

  // Find all mappings for this specific note/layer
  juce::Array<int> matchingIndices;
  for (int i = 0; i < libraryData.mappings.size(); ++i) {
    auto &m = libraryData.mappings.getReference(i);
    if (m.midiNote == note && m.micLayer == layer &&
        m.samplePath.isNotEmpty()) {
      matchingIndices.add(i);
    }
  }

  // The order of regions in col->regions matches the order added in
  // updateGridUI
  if (matchingIndices.size() == col->regions.size()) {
    for (int i = 0; i < matchingIndices.size(); ++i) {
      auto &m = libraryData.mappings.getReference(matchingIndices[i]);
      auto *region = col->regions[i];
      if (!region->isDragging()) {
        region->updateFromMapping(m);
      }
    }
  }

  col->resized(); // Trigger re-layout of the column's regions
}

void MainComponent::filesDropped(const juce::StringArray &files, int x, int y) {
  // Global drops are now handled by individual KeyColumns.
  // We keep this empty or for non-target drops if needed.
}

} // namespace sotero
