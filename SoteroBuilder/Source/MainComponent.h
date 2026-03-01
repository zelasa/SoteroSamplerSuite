#pragma once

#include "../../Common/SoteroFormat.h"
#include <JuceHeader.h>
#include <functional>
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

  // --- Grid Mapper Section ---
  struct LayerSlot : public juce::Component {
    int layerIndex = 0;
    int noteIndex = 0;
    bool hasSample = false;
    juce::String fileName;
    std::function<void()> onFileChanged;

    LayerSlot(int lIdx, int nIdx) : layerIndex(lIdx), noteIndex(nIdx) {}

    void paint(juce::Graphics &g) override {
      auto bounds = getLocalBounds().reduced(1).toFloat();
      g.setColour(hasSample ? juce::Colours::cyan.withAlpha(0.6f)
                            : juce::Colours::white.withAlpha(0.05f));
      g.fillRoundedRectangle(bounds, 3.0f);

      g.setColour(juce::Colours::white.withAlpha(hasSample ? 0.8f : 0.2f));
      g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

      if (hasSample) {
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawFittedText(fileName, getLocalBounds().reduced(2),
                         juce::Justification::centred, 2);
      } else {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.setFont(14.0f);
        g.drawText("+", getLocalBounds(), juce::Justification::centred);
      }
    }

    void mouseDown(const juce::MouseEvent &) override {
      if (onFileChanged)
        onFileChanged();
    }
  };

  struct KeyColumn : public juce::Component {
    juce::OwnedArray<LayerSlot> layers;
    int noteNumber = 60;

    KeyColumn(int note) : noteNumber(note) {
      for (int i = 0; i < 5; ++i) {
        layers.add(new LayerSlot(i, note - 60));
        addAndMakeVisible(layers.getLast());
      }
    }

    void resized() override {
      auto r = getLocalBounds();
      int h = r.getHeight() / 5;
      for (auto *l : layers)
        l->setBounds(r.removeFromTop(h));
    }
  };

  juce::OwnedArray<KeyColumn> keyColumns;
  juce::MidiKeyboardState keyboardState;
  std::unique_ptr<juce::MidiKeyboardComponent> keyboard;

  // --- Data ---
  LibraryMetadata libraryData;
  juce::File lastBrowseDirectory;

  void updateGridUI();
  void updateMetadataFromUI();

  // --- Actions ---
  juce::TextButton exportButton{"GENERATE .SPSA"};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
} // namespace sotero
