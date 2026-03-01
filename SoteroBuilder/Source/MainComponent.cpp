#include "MainComponent.h"
#include "../../Common/SoteroArchive.h"
#include "../../SamplerPlayer/Source/SoteroSamplerVoice.h"

namespace sotero {
MainComponent::MainComponent() {
  // 1. Initialize metadata for 12 keys (C to B), each with 5 velocity layers
  // Index 0 = Piano (0-24), Index 4 = Forte (100-127)
  // The UI (KeyColumn) will draw Index 4 at the TOP.
  for (int note = 0; note < 12; ++note) {
    for (int layer = 0; layer < 5; ++layer) {
      KeyMapping m;
      m.midiNote = 60 + note;
      m.velocityLow = layer * 25;
      m.velocityHigh = (layer == 4) ? 127 : (layer * 25 + 24);
      m.samplePath = "";
      m.fileName = "";
      m.chokeGroup = 0;
      libraryData.mappings.add(m);
    }
  }

  // 2. Header UI
  titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
  titleLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(titleLabel);

  nameEditor.setTextToShowWhenEmpty("Library Name", juce::Colours::grey);
  authorEditor.setTextToShowWhenEmpty("Author Name", juce::Colours::grey);
  addAndMakeVisible(nameEditor);
  addAndMakeVisible(authorEditor);

  // 3. Grid Columns (12 notes)
  for (int i = 0; i < 12; ++i) {
    auto *col = keyColumns.add(new KeyColumn(60 + i));

    for (int layer = 0; layer < 5; ++layer) {
      auto *slot = col->layers[layer];
      slot->onAudition = [this, i, layer] {
        int mappingIndex = i * 5 + layer;
        auto &mapping = libraryData.mappings.getReference(mappingIndex);
        if (mapping.samplePath.isNotEmpty())
          // Now Layer 4 is Top (Forte), Layer 0 is Bottom (Piano)
          auditionSample(mapping.samplePath, 60 + i, (layer * 25 + 12));
      };

      slot->onClear = [this, i, layer] {
        int mappingIndex = i * 5 + layer;
        auto &mapping = libraryData.mappings.getReference(mappingIndex);
        mapping.samplePath = "";
        mapping.fileName = "";
        updateGridUI();
        rebuildSynth(); // Update the audition engine
      };

      slot->onFileChanged = [this, i, layer] {
        chooser = std::make_unique<juce::FileChooser>(
            "Select a WAV file...", lastBrowseDirectory, "*.wav;*.aif;*.aiff");

        auto chooserFlags = juce::FileBrowserComponent::openMode |
                            juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, i, layer](
                                               const juce::FileChooser &fc) {
          auto file = fc.getResult();
          if (file.existsAsFile()) {
            lastBrowseDirectory = file.getParentDirectory();
            int mappingIndex = i * 5 + layer;
            auto &mapping = libraryData.mappings.getReference(mappingIndex);
            mapping.samplePath = file.getFullPathName();
            mapping.fileName = file.getFileName();
            updateGridUI();
            rebuildSynth(); // Update the audition engine

            // Auto-audition the dropped file
            auditionSample(file.getFullPathName(), 60 + i, (layer * 25 + 12));
          }
        });
      };
    }
    addAndMakeVisible(col);
  }

  // 4. Custom SemiTone Keyboard UI (Piano Roll Style)
  keyboard = std::make_unique<SemiToneKeyboard>();
  keyboard->onKeyPress = [this](int note) { synth.noteOn(1, note, 0.8f); };
  addAndMakeVisible(keyboard.get());

  // 5. Footer UI
  importButton.onClick = [this] {
    chooser = std::make_unique<juce::FileChooser>(
        "Open Sotero Library...", lastBrowseDirectory, "*.spsa;*.sotero");

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser &fc) {
          auto file = fc.getResult();
          if (file.existsAsFile()) {
            auto imported = sotero::SoteroArchive::readMetadata(file);
            if (imported.mappings.size() > 0) {
              currentLibraryFile = file;
              libraryData = imported;
              nameEditor.setText(libraryData.name);
              authorEditor.setText(libraryData.author);
              updateGridUI();
              rebuildSynth();
              juce::NativeMessageBox::showMessageBoxAsync(
                  juce::MessageBoxIconType::InfoIcon, "Success",
                  "Library imported successfully!");
            } else {
              juce::NativeMessageBox::showMessageBoxAsync(
                  juce::MessageBoxIconType::WarningIcon, "Error",
                  "Failed to read library file (Invalid format?).");
            }
          }
        });
  };
  addAndMakeVisible(importButton);

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
                               juce::NativeMessageBox::showMessageBoxAsync(
                                   juce::MessageBoxIconType::InfoIcon,
                                   "Success",
                                   "Sotero Library generated successfully!");
                             } else {
                               juce::NativeMessageBox::showMessageBoxAsync(
                                   juce::MessageBoxIconType::WarningIcon,
                                   "Error", "Failed to generate library.");
                             }
                           }
                         });
  };

  exportButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colours::orange.darker(0.5f));
  exportButton.setColour(juce::TextButton::textColourOffId,
                         juce::Colours::white);
  addAndMakeVisible(exportButton);

  // 6. Audio Initialization
  formatManager.registerBasicFormats();
  for (int i = 0; i < 8; ++i)
    synth.addVoice(new juce::SamplerVoice()); // Basic audition voices

  setAudioChannels(0, 2); // No inputs, 2 outputs

  lastBrowseDirectory =
      juce::File::getSpecialLocation(juce::File::userHomeDirectory);

  setSize(900, 700);

  // Register as MIDI callback
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  for (auto &device : midiInputs) {
    if (deviceManager.isMidiInputDeviceEnabled(device.identifier))
      deviceManager.addMidiInputDeviceCallback(device.identifier, this);
  }

  // Add voices for auditioning
  for (int i = 0; i < 16; ++i) {
    synth.addVoice(new sotero::SoteroSamplerVoice());
  }

  updateGridUI();
}

MainComponent::~MainComponent() {
  deviceManager.removeMidiInputDeviceCallback({}, this);
  shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  synth.setCurrentPlaybackSampleRate(sampleRate);
  midiCollector.reset(sampleRate);
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  bufferToFill.clearActiveBufferRegion();

  juce::MidiBuffer incomingMidi;
  midiCollector.removeNextBlockOfMessages(incomingMidi,
                                          bufferToFill.numSamples);

  synth.renderNextBlock(*bufferToFill.buffer, incomingMidi,
                        bufferToFill.startSample, bufferToFill.numSamples);
}

void MainComponent::handleIncomingMidiMessage(
    juce::MidiInput *source, const juce::MidiMessage &message) {
  midiCollector.addMessageToQueue(message);
}

void MainComponent::releaseResources() {}

void MainComponent::rebuildSynth() {
  const juce::ScopedLock sl(synthLock);
  synth.clearSounds();

  for (const auto &m : libraryData.mappings) {
    if (m.samplePath.isNotEmpty()) {
      std::unique_ptr<juce::AudioFormatReader> reader;

      juce::File file(m.samplePath);
      if (file.existsAsFile()) {
        // It's a local file path
        reader.reset(formatManager.createReaderFor(file));
      } else if (currentLibraryFile.existsAsFile()) {
        // Try to load as an internal archive reference
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

        synth.addSound(new sotero::SoteroSamplerSound(
            m.samplePath, *reader, range, m.midiNote, 0.01, 0.1, 10.0,
            m.chokeGroup, m.velocityLow, m.velocityHigh));
      }
    }
  }
}

void MainComponent::auditionSample(const juce::String &path, int midiNote,
                                   int velocity) {
  const juce::ScopedLock sl(synthLock);
  // Synth note on will trigger the sounds already loaded in rebuildSynth
  synth.noteOn(1, midiNote, (float)velocity / 127.0f);
}

void MainComponent::updateGridUI() {
  for (int i = 0; i < 12; ++i) {
    auto *col = keyColumns[i];
    for (int layer = 0; layer < 5; ++layer) {
      auto &mapping = libraryData.mappings.getReference(i * 5 + layer);
      auto *slot = col->layers[layer];
      slot->hasSample = mapping.samplePath.isNotEmpty();
      if (slot->hasSample) {
        slot->fileName = mapping.fileName.isNotEmpty()
                             ? mapping.fileName
                             : juce::File(mapping.samplePath).getFileName();
      }
      slot->repaint();
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

void MainComponent::resized() {
  auto r = getLocalBounds().reduced(20);

  // Header
  titleLabel.setBounds(r.removeFromTop(40));

  auto infoArea = r.removeFromTop(40);
  nameEditor.setBounds(infoArea.removeFromLeft(r.getWidth() / 2).reduced(5));
  authorEditor.setBounds(infoArea.reduced(5));

  r.removeFromTop(10);

  // Footer First
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

  // Grid Layout - Piano Roll Alignment
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
        for (int layer = 0; layer < 5; ++layer) {
          auto *slot = col->layers[layer];
          if (slot->getBounds().contains(x - colBounds.getX(), localY)) {
            for (int f = 0; f < files.size() && (layer + f) < 5; ++f) {
              int mappingIndex = i * 5 + (layer + f);
              auto &mapping = libraryData.mappings.getReference(mappingIndex);
              mapping.samplePath = files[f];
              mapping.fileName = juce::File(files[f]).getFileName();
            }
            updateGridUI();
            rebuildSynth();
            return;
          }
        }
      }
    }
  }
}

} // namespace sotero
