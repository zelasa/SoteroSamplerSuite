#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ADSRWidget.h"
#include "FilterWidget.h"

namespace sotero {

/**
 * @class SculptingWidget
 * @brief Widget for ADSR, Filter, and Velocity Sensitivity controls.
 */
class SculptingWidget : public juce::Component {
public:
    SculptingWidget() {
        addAndMakeVisible(title);
        addAndMakeVisible(adsrWidget);
        addAndMakeVisible(filterWidget);
        addAndMakeVisible(velSensSlider);
        
        title.setText("SCULPTING TOOLS", juce::dontSendNotification);
        velSensSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        velSensSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(10);
        title.setBounds(r.removeFromTop(25));
        
        auto adsrArea = r.removeFromLeft(r.getWidth() * 0.6f).reduced(5);
        adsrWidget.setBounds(adsrArea);
        
        auto filterArea = r.removeFromTop(r.getHeight() * 0.7f).reduced(5);
        filterWidget.setBounds(filterArea);
        
        velSensSlider.setBounds(r.reduced(5));
    }

    void paint(juce::Graphics &g) override {
        auto r = getLocalBounds().reduced(2);
        g.setColour(juce::Colour(0xff0a0a0a));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(r.toFloat(), 4.0f, 1.0f);
    }

    ADSRWidget& getADSR() { return adsrWidget; }
    FilterWidget& getFilter() { return filterWidget; }
    juce::Slider& getVelSensSlider() { return velSensSlider; }

private:
    ADSRWidget adsrWidget;
    FilterWidget filterWidget;
    juce::Slider velSensSlider;
    juce::Label title;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SculptingWidget)
};

} // namespace sotero
