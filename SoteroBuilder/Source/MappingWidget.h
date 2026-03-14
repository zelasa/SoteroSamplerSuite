#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SampleRegion.h"
#include "../../Common/SoteroFormat.h"
#include "../../Common/UI/SoteroLookAndFeel.h"

namespace sotero {

/**
 * @class KeyColumn
 * @brief Represents a single MIDI note column in the mapping grid.
 */
class KeyColumn : public juce::Component, public juce::FileDragAndDropTarget {
public:
    KeyColumn(int note, int idx, int layer)
        : noteNumber(note), colIndex(idx), micLayer(layer) {}

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Alternating background
        auto grey = (colIndex % 2 == 0) ? juce::Colours::white.withAlpha(0.05f) 
                                        : juce::Colours::white.withAlpha(0.02f);
        g.setColour(grey);
        g.fillRoundedRectangle(bounds, 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
    }

    void resized() override {
        for (auto *r : regions) {
            float h = (float)getHeight();
            float yTop = h * (1.0f - (r->getMapping().velocityHigh / 127.0f));
            float yBot = h * (1.0f - (r->getMapping().velocityLow / 127.0f));
            r->setBounds(0, (int)yTop, getWidth(), (int)juce::jmax(1.0f, yBot - yTop));
        }
    }

    void mouseDown(const juce::MouseEvent&) override {
        if (onBackgroundClick) onBackgroundClick();
    }

    // --- FileDragAndDropTarget Implementation ---
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void filesDropped(const juce::StringArray& files, int x, int y) override {
        if (onFilesDropped) onFilesDropped(files, y);
    }

    void addRegion(const KeyMapping &m, int mappingIndex) {
        auto *r = new SampleRegion(m, noteNumber, micLayer, mappingIndex);
        regions.add(r);
        addAndMakeVisible(r);
        resized();
    }

    void clearRegions() {
        regions.clear();
    }

    std::function<void(const juce::StringArray&, int)> onFilesDropped;
    std::function<void()> onBackgroundClick;

    int noteNumber;
    int colIndex;
    int micLayer;
    juce::OwnedArray<SampleRegion> regions;
};

/**
 * @class SemiToneKeyboard
 * @brief Simple horizontal keyboard display for the mapping grid.
 */
class SemiToneKeyboard : public juce::Component {
public:
    std::function<void(int)> onKeyPress;
    std::function<void()> onBackgroundClick;

    void paint(juce::Graphics &g) override {
        auto r = getLocalBounds().toFloat();
        float keyWidth = r.getWidth() / 12.0f;

        for (int i = 0; i < 12; ++i) {
            auto keyRect = juce::Rectangle<float>(i * keyWidth, 0, keyWidth, r.getHeight()).reduced(0.5f);
            bool isBlack = juce::MidiMessage::isMidiNoteBlack(60 + i);

            if (isBlack) {
                g.setColour(juce::Colours::white.withAlpha(0.8f));
                g.fillRect(keyRect);
                auto cap = keyRect.withHeight(r.getHeight() * 0.65f).reduced(2.0f, 0.0f);
                g.setColour(juce::Colours::black);
                g.fillRoundedRectangle(cap, 2.0f);
            } else {
                g.setColour(juce::Colours::white);
                g.fillRoundedRectangle(keyRect, 2.0f);
            }

            g.setColour(juce::Colours::grey.withAlpha(0.3f));
            g.drawRoundedRectangle(keyRect, 2.0f, 1.0f);

            g.setColour(isBlack ? juce::Colours::grey : juce::Colours::black);
            g.setFont(10.0f);
            g.drawText(juce::MidiMessage::getMidiNoteName(60 + i, true, false, 3),
                       keyRect.removeFromBottom(15), juce::Justification::centred);
        }
    }

    void mouseDown(const juce::MouseEvent &e) override {
        float keyWidth = (float)getWidth() / 12.0f;
        int note = (int)(e.x / keyWidth);
        if (onKeyPress) onKeyPress(60 + note);
        else if (onBackgroundClick) onBackgroundClick();
    }
};

/**
 * @class MappingWidget
 * @brief Complex grid mapping widget for sample placement.
 */
class MappingWidget : public juce::Component {
public:
    struct LayerView : public juce::Component {
        juce::OwnedArray<KeyColumn> columns;
        std::unique_ptr<SemiToneKeyboard> keyboard;
        juce::Label label;
        juce::ToggleButton toPlayerToggle{"TO PLAYER"};
        juce::Colour themeColor;
        int micLayer;

        LayerView(juce::Colour c, juce::String name, int layerIdx)
            : micLayer(layerIdx), themeColor(c) {
            label.setText(name, juce::dontSendNotification);
            label.setColour(juce::Label::textColourId, c);
            addAndMakeVisible(label);
            addAndMakeVisible(toPlayerToggle);

            for (int i = 0; i < 12; ++i)
                columns.add(new KeyColumn(60 + i, i, micLayer));

            for (auto *col : columns)
                addAndMakeVisible(col);

            keyboard = std::make_unique<SemiToneKeyboard>();
            addAndMakeVisible(keyboard.get());
        }

        void paint(juce::Graphics &g) override {
            auto r = getLocalBounds().toFloat();
            g.setColour(SoteroLookAndFeel::getWidgetBackground());
            g.fillRoundedRectangle(r, 4.0f);
            g.setColour(themeColor.withAlpha(0.3f));
            g.drawRoundedRectangle(r, 4.0f, 1.0f);
        }

        void resized() override {
            auto r = getLocalBounds();
            auto headerArea = r.removeFromTop(25);
            toPlayerToggle.setBounds(headerArea.removeFromRight(100).reduced(2));
            label.setBounds(headerArea.reduced(5, 0));
            auto bottomArea = r.removeFromBottom(40);
            keyboard->setBounds(bottomArea);

            float colW = (float)r.getWidth() / 12.0f;
            for (int i = 0; i < 12; ++i)
                columns[i]->setBounds(juce::roundToInt(i * colW), r.getY(),
                                      juce::roundToInt(colW), r.getHeight());
        }
    };

    MappingWidget() {
        layerSyncLock.setTooltip("When ON, moving or resizing a sample in one "
                                 "layer will replicate to the other.");
        layerSyncLock.setColour(juce::ToggleButton::textColourId, SoteroLookAndFeel::getOrangeAccent());
        addAndMakeVisible(layerSyncLock);

        layer1 = std::make_unique<LayerView>(SoteroLookAndFeel::getLayer1Colour(), "LAYER 1 (MIC 1)", 0);
        layer2 = std::make_unique<LayerView>(SoteroLookAndFeel::getLayer2Colour(), "LAYER 2 (MIC 2)", 1);
        
        addAndMakeVisible(layer1.get());
        addAndMakeVisible(layer2.get());
        addAndMakeVisible(octaveSelector);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(5);
        auto ctrlBar = r.removeFromBottom(25);
        octaveSelector.setBounds(ctrlBar.removeFromLeft(125).reduced(2));
        layerSyncLock.setBounds(ctrlBar.removeFromLeft(125).reduced(2));

        int gap = 24;
        auto layerArea = r;
        int w = (layerArea.getWidth() - gap) / 2;

        if (layer1) layer1->setBounds(layerArea.removeFromLeft(w).reduced(2));
        layerArea.removeFromLeft(gap);
        if (layer2) layer2->setBounds(layerArea.reduced(2));
    }

    void updateOctave(int newOctave) {
        currentOctave = newOctave;
        int baseNote = (currentOctave + 3) * 12;
        if (layer1) {
            for (int i = 0; i < 12; ++i)
                layer1->columns[i]->noteNumber = baseNote + i;
        }
        if (layer2) {
            for (int i = 0; i < 12; ++i)
                layer2->columns[i]->noteNumber = baseNote + i;
        }
    }

    std::unique_ptr<LayerView> layer1, layer2;
    juce::ComboBox octaveSelector;
    juce::ToggleButton layerSyncLock{"SYNC LAYERS"};
    int currentOctave = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MappingWidget)
};

} // namespace sotero
