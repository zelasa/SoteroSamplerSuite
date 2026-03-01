#pragma once

#include "../../Common/SoteroFormat.h"
#include <JuceHeader.h>
#include <memory>

namespace sotero {
/**
 * @class MainComponent
 * @brief The main workspace for SoteroBuilder.
 */
class MainComponent : public juce::Component,
                      public juce::FileDragAndDropTarget {
public:
  MainComponent();
  ~MainComponent() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  // --- FileDragAndDropTarget Overrides ---
  bool isInterestedInFileDrag(const juce::StringArray &files) override {
    return true;
  }
  void filesDropped(const juce::StringArray &files, int x, int y) override;

private:
  std::unique_ptr<juce::FileChooser> chooser;
  // --- Library Info Section ---
  juce::Label titleLabel{"BuilderTitle", "SOTERO BUILDER"};
  juce::TextEditor nameEditor;
  juce::TextEditor authorEditor;

  // --- Key Selector Section ---
  juce::OwnedArray<juce::TextButton> keyButtons;
  int selectedKeyIndex = 0;

  // --- Layer Mapper Section ---
  struct LayerSlot : public juce::Component {
    juce::Label layerLabel;
    juce::Label pathLabel;
    juce::TextButton browseButton{"BROWSE"};
    juce::ComboBox chokeCombo;

    LayerSlot(int index) {
      layerLabel.setText("LAYER " + juce::String(index + 1),
                         juce::dontSendNotification);
      pathLabel.setText("No file selected...", juce::dontSendNotification);
      addAndMakeVisible(layerLabel);
      addAndMakeVisible(pathLabel);
      addAndMakeVisible(browseButton);

      chokeCombo.addItem("No Choke", 1);
      for (int i = 1; i <= 8; ++i)
        chokeCombo.addItem("Choke Group " + juce::String(i), i + 1);
      chokeCombo.setSelectedId(1);
      addAndMakeVisible(chokeCombo);
    }

    void resized() override {
      auto r = getLocalBounds().reduced(5);
      layerLabel.setBounds(r.removeFromLeft(80));
      browseButton.setBounds(r.removeFromRight(80));
      chokeCombo.setBounds(r.removeFromRight(120).reduced(2));
      pathLabel.setBounds(r);
    }
  };

  juce::OwnedArray<LayerSlot> layerSlots;

  // --- Data ---
  LibraryMetadata libraryData;

  void updateLayerUI();
  void updateMetadataFromUI();
  void selectKey(int index);

  // --- Actions ---
  juce::TextButton exportButton{"GENERATE .SPSA"};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
} // namespace sotero
