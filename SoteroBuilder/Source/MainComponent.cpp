#include "MainComponent.h"
#include "../../Common/SoteroArchive.h"

namespace sotero {
MainComponent::MainComponent() {
  // 1. Initialize metadata for 12 keys (C to B), each with 5 velocity layers
  for (int note = 0; note < 12; ++note) {
    for (int layer = 0; layer < 5; ++layer) {
      KeyMapping m;
      m.midiNote = 60 + note;
      m.velocityLow = layer * 25;
      m.velocityHigh = (layer == 4) ? 127 : (layer * 25 + 24);
      m.samplePath = "";
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
      slot->onFileChanged = [this, i, layer] {
        chooser = std::make_unique<juce::FileChooser>(
            "Select a WAV file...", lastBrowseDirectory, "*.wav;*.aif;*.aiff");

        auto chooserFlags = juce::FileBrowserComponent::openMode |
                            juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(
            chooserFlags, [this, i, layer](const juce::FileChooser &fc) {
              auto file = fc.getResult();
              if (file.existsAsFile()) {
                lastBrowseDirectory = file.getParentDirectory();
                int mappingIndex = i * 5 + layer;
                libraryData.mappings.getReference(mappingIndex).samplePath =
                    file.getFullPathName();
                updateGridUI();
              }
            });
      };
    }
    // Note: We add them all but will manage Z-order in resized or by order of
    // addition
    addAndMakeVisible(col);
  }

  // 4. Standard Keyboard UI
  keyboard = std::make_unique<juce::MidiKeyboardComponent>(
      keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard);
  keyboard->setAvailableRange(60, 71);
  keyboard->setScrollButtonsVisible(false);
  addAndMakeVisible(keyboard.get());

  // 5. Footer UI
  exportButton.onClick = [this] {
    updateMetadataFromUI();

    chooser = std::make_unique<juce::FileChooser>(
        "Save Sotero Library...",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile(libraryData.name + ".spsa"),
        "*.spsa;*.sotero");

    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser &fc) {
          auto file = fc.getResult();
          if (file != juce::File{}) {
            if (sotero::SoteroArchive::write(file, libraryData)) {
              juce::NativeMessageBox::showMessageBoxAsync(
                  juce::MessageBoxIconType::InfoIcon, "Success",
                  "Sotero Library generated successfully!");
            } else {
              juce::NativeMessageBox::showMessageBoxAsync(
                  juce::MessageBoxIconType::WarningIcon, "Error",
                  "Failed to generate library.");
            }
          }
        });
  };

  exportButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colours::orange.darker(0.5f));
  exportButton.setColour(juce::TextButton::textColourOffId,
                         juce::Colours::white);
  addAndMakeVisible(exportButton);

  lastBrowseDirectory =
      juce::File::getSpecialLocation(juce::File::userHomeDirectory);

  setSize(900, 700);
  updateGridUI();
}

MainComponent::~MainComponent() {}

void MainComponent::updateGridUI() {
  for (int i = 0; i < 12; ++i) {
    auto *col = keyColumns[i];
    for (int layer = 0; layer < 5; ++layer) {
      auto &mapping = libraryData.mappings.getReference(i * 5 + layer);
      auto *slot = col->layers[layer];
      slot->hasSample = !mapping.samplePath.isEmpty();
      if (slot->hasSample) {
        slot->fileName = juce::File(mapping.samplePath).getFileName();
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
  exportButton.setBounds(footerArea.withSizeKeepingCentre(200, 40));

  keyboard->setBounds(r.removeFromBottom(100));

  r.removeFromBottom(5);

  // Grid Layout - Proportional Alignment with Standard Keyboard
  // Note: We MUST use the keyboard's white key width to align
  float whiteKeyWidth =
      keyboard->getWidth() / 7.0f; // 7 white keys in an octave

  for (int i = 0; i < 12; ++i) {
    auto keyRect = keyboard->getRectangleForKey(60 + i);

    // Position the column exactly over the key
    keyColumns[i]->setBounds(keyboard->getX() + (int)keyRect.getX(), r.getY(),
                             (int)keyRect.getWidth(), r.getHeight());
  }

  // Z-Order: Ensure black keys are on top of white keys for correct overlapping
  // visualization
  for (int i = 0; i < 12; ++i) {
    if (juce::MidiMessage::isMidiNoteBlack(60 + i)) {
      keyColumns[i]->toFront(false);
    }
  }
}

void MainComponent::filesDropped(const juce::StringArray &files, int x, int y) {
  if (files.size() > 0) {
    // Check black keys FIRST (higher Z-order/overlapping)
    for (int i = 0; i < keyColumns.size(); ++i) {
      int idx = 11 - i; // Reverse check for simple top-down priority? No,
                        // explicitly check blacks.
    }

    // Structured Priority: Black keys then White keys
    auto checkIndex = [this, files, x, y](int i) -> bool {
      auto *col = keyColumns[i];
      auto colBounds = col->getBounds();
      if (colBounds.contains(x, y)) {
        int localY = y - colBounds.getY();
        for (int layer = 0; layer < 5; ++layer) {
          auto *slot = col->layers[layer];
          if (slot->getBounds().contains(x - colBounds.getX(), localY)) {
            for (int f = 0; f < files.size() && (layer + f) < 5; ++f) {
              int mappingIndex = i * 5 + (layer + f);
              libraryData.mappings.getReference(mappingIndex).samplePath =
                  files[f];
            }
            updateGridUI();
            return true;
          }
        }
      }
      return false;
    };

    // 1. Check Blacks
    for (int i = 0; i < 12; ++i) {
      if (juce::MidiMessage::isMidiNoteBlack(60 + i)) {
        if (checkIndex(i))
          return;
      }
    }
    // 2. Check Whites
    for (int i = 0; i < 12; ++i) {
      if (!juce::MidiMessage::isMidiNoteBlack(60 + i)) {
        if (checkIndex(i))
          return;
      }
    }
  }
}

} // namespace sotero
