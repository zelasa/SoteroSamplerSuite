#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace sotero {

/**
 * @class WaveformWidget
 * @brief Visualizer for audio resources.
 */
class WaveformWidget : public juce::Component {
public:
    WaveformWidget(juce::Colour c, juce::String t) : bgColor(c), title(t) {
        addAndMakeVisible(toPlayerToggle);
        toPlayerToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    }

    void paint(juce::Graphics &g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff181818)); 
        g.fillRoundedRectangle(r, 4.0f);
        
        // Restore background color fill
        g.setColour(bgColor.withAlpha(0.12f));
        g.fillRoundedRectangle(r, 4.0f);

        g.setColour(bgColor.withAlpha(0.2f));
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
       
        g.setColour(juce::Colours::yellow.withAlpha(0.6f));
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawFittedText(title, getLocalBounds().reduced(5), juce::Justification::centredTop, 1);
       
        // Waveform placeholder logic
        g.setColour(bgColor.withAlpha(0.5f));
        auto waveR = getLocalBounds().reduced(10, 30);
        g.drawHorizontalLine(waveR.getCentreY(), (float)waveR.getX(), (float)waveR.getRight());
    }

    void resized() override {
        toPlayerToggle.setBounds(getWidth() - 100, 5, 90, 20);
    }

    juce::ToggleButton& getToPlayerToggle() { return toPlayerToggle; }

private:
    juce::Colour bgColor;
    juce::String title;
    juce::ToggleButton toPlayerToggle{"TO PLAYER"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformWidget)
};

} // namespace sotero
