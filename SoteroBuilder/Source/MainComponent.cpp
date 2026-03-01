#include "MainComponent.h"
#include "../../Common/SoteroArchive.h"

namespace sotero {
MainComponent::MainComponent() {
  // Initialize metadata for 12 keys (C to B), each with 5 velocity layers
  for (int note = 0; note < 12; ++note) {
    for (int layer = 0; layer < 5; ++layer) {
      KeyMapping m;
      m.midiNote = 60 + note; // Middle C onwards for now
      m.velocityLow = layer * 25;
      m.velocityHigh = (layer == 4) ? 127 : (layer * 25 + 24);
      m.samplePath = "";
      m.chokeGroup = 0;
      libraryData.mappings.add(m);
    }
  }

  // Styling Label
  titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
  titleLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(titleLabel);

  // Name & Author Setup
  nameEditor.setTextToShowWhenEmpty("Library Name", juce::Colours::grey);
  authorEditor.setTextToShowWhenEmpty("Author Name", juce::Colours::grey);
  addAndMakeVisible(nameEditor);
  addAndMakeVisible(authorEditor);

  // Key Selector (12 buttons)
  const juce::StringArray noteNames = {"C",  "C#", "D",  "D#", "E",  "F",
                                       "F#", "G",  "G#", "A",  "A#", "B"};
  for (int i = 0; i < 12; ++i) {
    auto *b = keyButtons.add(new juce::TextButton(noteNames[i]));
    b->setRadioGroupId(100);
    b->setClickingTogglesState(true);
    if (i == 0)
      b->setToggleState(true, juce::dontSendNotification);

    b->onClick = [this, i] { selectKey(i); };
    addAndMakeVisible(b);
  }

  // Layer Slots (5 layers)
  for (int i = 0; i < 5; ++i) {
    auto *slot = layerSlots.add(new LayerSlot(i));

    slot->browseButton.onClick = [this, i] {
      chooser = std::make_unique<juce::FileChooser>(
          "Select a WAV file...",
          juce::File::getSpecialLocation(juce::File::userHomeDirectory),
          "*.wav;*.aif;*.aiff");

      auto chooserFlags = juce::FileBrowserComponent::openMode |
                          juce::FileBrowserComponent::canSelectFiles;

      chooser->launchAsync(
          chooserFlags, [this, i](const juce::FileChooser &fc) {
            auto file = fc.getResult();
            if (file.existsAsFile()) {
              int mappingIndex = selectedKeyIndex * 5 + i;
              libraryData.mappings.getReference(mappingIndex).samplePath =
                  file.getFullPathName();
              updateLayerUI();
            }
          });
    };

    slot->chokeCombo.onChange = [this, i] {
      int mappingIndex = selectedKeyIndex * 5 + i;
      libraryData.mappings.getReference(mappingIndex).chokeGroup =
          layerSlots[i]->chokeCombo.getSelectedId() - 1;
    };

    addAndMakeVisible(slot);
  }

  // Export Button
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

  setSize(800, 600);
  updateLayerUI();
}

MainComponent::~MainComponent() {}

void MainComponent::selectKey(int index) {
  selectedKeyIndex = index;
  updateLayerUI();
}

void MainComponent::updateLayerUI() {
  // Update the 5 slots with the data for the currently selected key
  int startIndex = selectedKeyIndex * 5;
  for (int i = 0; i < 5; ++i) {
    auto &mapping = libraryData.mappings.getReference(startIndex + i);
    layerSlots[i]->pathLabel.setText(mapping.samplePath.isEmpty()
                                         ? "No file selected..."
                                         : mapping.samplePath,
                                     juce::dontSendNotification);
    layerSlots[i]->chokeCombo.setSelectedId(mapping.chokeGroup + 1,
                                            juce::dontSendNotification);
  }
}

void MainComponent::updateMetadataFromUI() {
  libraryData.name = nameEditor.getText();
  libraryData.author = authorEditor.getText();
}

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff121212)); // Premium Dark Background

  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRect(getLocalBounds().reduced(10), 2.0f);
}

void MainComponent::resized() {
  auto r = getLocalBounds().reduced(20);

  // Header
  titleLabel.setBounds(r.removeFromTop(40));
  r.removeFromTop(10);

  auto infoArea = r.removeFromTop(40);
  nameEditor.setBounds(infoArea.removeFromLeft(r.getWidth() / 2).reduced(5));
  authorEditor.setBounds(infoArea.reduced(5));

  r.removeFromTop(20);

  // Key Selector
  auto keyArea = r.removeFromTop(50);
  int btnWidth = keyArea.getWidth() / 12;
  for (auto *b : keyButtons)
    b->setBounds(keyArea.removeFromLeft(btnWidth).reduced(2));

  r.removeFromTop(20);

  // Layer Mapper
  for (auto *slot : layerSlots) {
    slot->setBounds(r.removeFromTop(50).reduced(2));
  }

  // Footer
  r.removeFromTop(20);
  exportButton.setBounds(r.removeFromBottom(50).withSizeKeepingCentre(200, 50));
}
void MainComponent::filesDropped(const juce::StringArray &files, int x, int y) {
  if (files.size() > 0) {
    // For now, map the first dropped file to Layer 1 of the selected key
    int mappingIndex = selectedKeyIndex * 5;
    libraryData.mappings.getReference(mappingIndex).samplePath = files[0];
    updateLayerUI();
  }
}

} // namespace sotero
