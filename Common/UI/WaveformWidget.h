#pragma once

#include "SoteroLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace sotero {

/**
 * @class WaveformWidget
 * @brief Widget for waveform visualization of a specific mic layer.
 */
class WaveformWidget : public juce::Component {
public:
    WaveformWidget(int layer) : micLayer(layer) {
        addAndMakeVisible(toPlayerToggle);
        
        // Use default LookAndFeel styling for the toggle
        toPlayerToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white.withAlpha(0.7f));
        
        title = (micLayer == 0) ? "MIC LAYER A" : "MIC LAYER B";
    }

    void paint(juce::Graphics &g) override {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        
        // Use LookAndFeel layer colors for specific background tint
        auto baseColor = (micLayer == 0) ? SoteroLookAndFeel::getLayer1Colour() 
                                         : SoteroLookAndFeel::getLayer2Colour();
        
        g.setColour(SoteroLookAndFeel::getWidgetBackground());
        g.fillRoundedRectangle(r, 4.0f);
        
        g.setColour(baseColor.withAlpha(0.12f));
        g.fillRoundedRectangle(r, 4.0f);

        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
       
        // Title
        g.setColour(SoteroLookAndFeel::getYellowAccent().withAlpha(0.8f));
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawFittedText(title, getLocalBounds().reduced(5), juce::Justification::centredTop, 1);
       
        // Waveform placeholder logic
        g.setColour(baseColor.withAlpha(0.4f));
        auto waveR = getLocalBounds().reduced(10, 30);
        g.drawHorizontalLine((float)waveR.getCentreY(), (float)waveR.getX(), (float)waveR.getRight());
        
        // Simple placeholder "blips"
        for (int i = 0; i < 20; ++i) {
            float x = waveR.getX() + (i * waveR.getWidth() / 20.0f);
            float h = 10.0f + (float)(std::rand() % 20);
            g.drawVerticalLine((int)x, waveR.getCentreY() - h/2.0f, waveR.getCentreY() + h/2.0f);
        }
    }

    void resized() override {
        toPlayerToggle.setBounds(getWidth() - 100, 5, 90, 20);
    }

    juce::ToggleButton& getToPlayerToggle() { return toPlayerToggle; }

private:
    int micLayer;
    juce::String title;
    juce::ToggleButton toPlayerToggle{"TO PLAYER"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformWidget)
};

} // namespace sotero
