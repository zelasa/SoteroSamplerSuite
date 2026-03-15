#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../SoteroEngineInterface.h"
#include "SoteroLookAndFeel.h"

namespace sotero {

/**
 * @class LoopSlot
 * @brief A single trigger box for a MIDI loop.
 */
class LoopSlot : public juce::Component, public juce::SettableTooltipClient {
public:
    LoopSlot(int idx, ISoteroAudioEngine& e) : index(idx), engine(e) {
        setTooltip("Click to trigger/stop MIDI loop");
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().reduced(2);
        bool isActive = false; // We would ideally poll this from engine or have a callback
        
        g.setColour(isActive ? SoteroLookAndFeel::getYellowAccent() : juce::Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        
        g.setColour(isActive ? juce::Colours::black : juce::Colours::white.withAlpha(0.6f));
        g.drawRoundedRectangle(r.toFloat(), 4.0f, 1.0f);
        
        g.setFont(10.0f);
        g.drawText(juce::String(index + 1), r, juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        triggered = !triggered;
        engine.triggerLoop(index, triggered);
        repaint();
    }

private:
    int index;
    ISoteroAudioEngine& engine;
    bool triggered = false;
};

/**
 * @class LoopMainControls
 * @brief Main UI for managing and triggering MIDI loops.
 */
class LoopMainControls : public juce::Component {
public:
    LoopMainControls(ISoteroAudioEngine& e) : engine(e) {
        addAndMakeVisible(cancellationToggle);
        cancellationToggle.setButtonText("GLOBAL CANCELLATION");
        cancellationToggle.onClick = [this] {
            engine.setLoopCancellationMode(cancellationToggle.getToggleState());
        };

        for (int i = 0; i < 3; ++i) {
            auto btn = abaBtns.add(new juce::TextButton(juce::String::charToString('A' + i)));
            addAndMakeVisible(btn);
            btn->setRadioGroupId(123);
            btn->setClickingTogglesState(true);
            btn->onClick = [this, i] { setAba(i); };
        }
        abaBtns[0]->setToggleState(true, juce::sendNotification);

        for (int i = 0; i < 12; ++i) {
            slots.add(new LoopSlot(i, engine));
            addAndMakeVisible(slots.getLast());
        }
        
        setAba(0);
    }

    void setAba(int abaIdx) {
        currentAba = abaIdx;
        // In a real implementation, we would update the slot indices
        // For simplicity here, we'll just repaint and assume the 12 slots
        // map to currentAba * 12 + i
        repaint();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black.withAlpha(0.9f));
        g.setColour(SoteroLookAndFeel::getYellowAccent().withAlpha(0.5f));
        g.drawRect(getLocalBounds(), 1);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(10);
        auto top = r.removeFromTop(30);
        cancellationToggle.setBounds(top.removeFromRight(150));
        
        auto abaArea = top.removeFromLeft(120);
        int bw = abaArea.getWidth() / 3;
        for (auto* b : abaBtns) b->setBounds(abaArea.removeFromLeft(bw).reduced(2));

        r.removeFromTop(10); // Gap

        int rows = 3;
        int cols = 4;
        int sw = r.getWidth() / cols;
        int sh = r.getHeight() / rows;

        for (int i = 0; i < 12; ++i) {
            int row = i / cols;
            int col = i % cols;
            slots[i]->setBounds(r.getX() + col * sw, r.getY() + row * sh, sw, sh);
            slots[i]->setBounds(slots[i]->getBounds().reduced(4));
        }
    }

private:
    ISoteroAudioEngine& engine;
    juce::ToggleButton cancellationToggle;
    juce::OwnedArray<juce::TextButton> abaBtns;
    juce::OwnedArray<LoopSlot> slots;
    int currentAba = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopMainControls)
};

} // namespace sotero
