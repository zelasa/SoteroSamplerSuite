#include "ADSRWidget.h"

namespace sotero {

ADSRWidget::ADSRWidget() {
    addAndMakeVisible(visualizer);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);

        s.onValueChange = [this] {
            if (onADSRChanged)
                onADSRChanged((float)attackSlider.getValue(),
                              (float)decaySlider.getValue(),
                              (float)sustainSlider.getValue(),
                              (float)releaseSlider.getValue());
        };
    };

    setupSlider(attackSlider, attackLabel, "A");
    setupSlider(decaySlider, decayLabel, "D");
    setupSlider(sustainSlider, sustainLabel, "S");
    setupSlider(releaseSlider, releaseLabel, "R");

    attackSlider.setRange(0.001, 5.0, 0.001);
    decaySlider.setRange(0.001, 5.0, 0.001);
    sustainSlider.setRange(0.0, 1.0, 0.01);
    releaseSlider.setRange(0.001, 5.0, 0.001);

    // Sync visualizer backwards if user drags the graph nodes
    visualizer.onAttackChange = [this](float v) { attackSlider.setValue(v, juce::dontSendNotification); if(onADSRChanged) onADSRChanged(v, (float)decaySlider.getValue(), (float)sustainSlider.getValue(), (float)releaseSlider.getValue()); };
    visualizer.onDecayChange = [this](float v) { decaySlider.setValue(v, juce::dontSendNotification); if(onADSRChanged) onADSRChanged((float)attackSlider.getValue(), v, (float)sustainSlider.getValue(), (float)releaseSlider.getValue()); };
    visualizer.onSustainChange = [this](float v) { sustainSlider.setValue(v, juce::dontSendNotification); if(onADSRChanged) onADSRChanged((float)attackSlider.getValue(), (float)decaySlider.getValue(), v, (float)releaseSlider.getValue()); };
    visualizer.onReleaseChange = [this](float v) { releaseSlider.setValue(v, juce::dontSendNotification); if(onADSRChanged) onADSRChanged((float)attackSlider.getValue(), (float)decaySlider.getValue(), (float)sustainSlider.getValue(), v); };
}

void ADSRWidget::setParams(float a, float d, float s, float r, 
                           float aC, float dC, float rC, 
                           float peak, float sTime) {
    attackSlider.setValue(a, juce::dontSendNotification);
    decaySlider.setValue(d, juce::dontSendNotification);
    sustainSlider.setValue(s, juce::dontSendNotification);
    releaseSlider.setValue(r, juce::dontSendNotification);
    
    visualizer.setParams(a, d, s, r, aC, dC, rC, peak, sTime);
}

void ADSRWidget::paint(juce::Graphics& g) {
    // Optional aesthetic border
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);
}

void ADSRWidget::resized() {
    auto r = getLocalBounds();
    
    // Bottom area for sliders
    auto sliderArea = r.removeFromBottom(60);
    float w = (float)sliderArea.getWidth() / 4.0f;
    
    auto aArea = sliderArea.removeFromLeft((int)w);
    attackLabel.setBounds(aArea.removeFromBottom(15));
    attackSlider.setBounds(aArea);
    
    auto dArea = sliderArea.removeFromLeft((int)w);
    decayLabel.setBounds(dArea.removeFromBottom(15));
    decaySlider.setBounds(dArea);
    
    auto sArea = sliderArea.removeFromLeft((int)w);
    sustainLabel.setBounds(sArea.removeFromBottom(15));
    sustainSlider.setBounds(sArea);
    
    auto rlArea = sliderArea;
    releaseLabel.setBounds(rlArea.removeFromBottom(15));
    releaseSlider.setBounds(rlArea);
    
    // Remaining area for the visualizer
    visualizer.setBounds(r.reduced(5));
}

} // namespace sotero
