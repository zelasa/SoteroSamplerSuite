#include "ADSRWidget.h"

namespace sotero {

ADSRWidget::ADSRWidget() {
    addAndMakeVisible(visualizer);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setMouseDragSensitivity(350); // Finer, more professional control
        addAndMakeVisible(s);
 
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
 
        s.onValueChange = [this] {
            float a = (float)attackSlider.getTimeValue();
            float d = (float)decaySlider.getTimeValue();
            float s = (float)sustainSlider.getTimeValue();
            float r = (float)releaseSlider.getTimeValue();
            float sTime = (float)sustainTimeSlider.getTimeValue();
  
            visualizer.updateADSR(a, d, s, r, sTime);
  
            if (onADSRChanged)
                onADSRChanged(a, d, s, r, sTime);
        };
    };

    setupSlider(attackSlider, attackLabel, "A");
    setupSlider(decaySlider, decayLabel, "D");
    setupSlider(sustainSlider, sustainLabel, "S");
    setupSlider(releaseSlider, releaseLabel, "R");
    setupSlider(sustainTimeSlider, sustainTimeLabel, "S.T");

    attackSlider.setTimeRange(0.001, 5.0, 0.001);
    decaySlider.setTimeRange(0.001, 5.0, 0.001);
    sustainSlider.setTimeRange(0.0, 1.0, 0.01); // Corrected range for level
    releaseSlider.setTimeRange(0.001, 5.0, 0.001);
    sustainTimeSlider.setTimeRange(0.1, 5.0, 0.01);

    // Initial value synchronization to prevent "ghost" movements
    attackSlider.setTimeValue(visualizer.getAttack());
    decaySlider.setTimeValue(visualizer.getDecay());
    sustainSlider.setTimeValue(visualizer.getSustain());
    releaseSlider.setTimeValue(visualizer.getRelease());
    sustainTimeSlider.setTimeValue(visualizer.getSustainTime());

    // Curve overrides via dedicated State-Aware Sliders
    attackSlider.onCurveChanged = [this](float val) {
        visualizer.setParams(visualizer.getAttack(), visualizer.getDecay(), 
                             visualizer.getSustain(), visualizer.getRelease(),
                             val, visualizer.getDecayCurve(), visualizer.getReleaseCurve(),
                             1.0f, visualizer.getSustainTime());
        if (visualizer.onAttackCurveChange) visualizer.onAttackCurveChange(val);
    };
    attackSlider.onSlopeModeChanged = [this](bool active) {
        attackLabel.setText(active ? "A. Slope" : "A", juce::dontSendNotification);
    };

    decaySlider.onCurveChanged = [this](float val) {
        visualizer.setParams(visualizer.getAttack(), visualizer.getDecay(), 
                             visualizer.getSustain(), visualizer.getRelease(),
                             visualizer.getAttackCurve(), val, visualizer.getReleaseCurve(),
                             1.0f, visualizer.getSustainTime());
        if (visualizer.onDecayCurveChange) visualizer.onDecayCurveChange(val);
    };
    decaySlider.onSlopeModeChanged = [this](bool active) {
        decayLabel.setText(active ? "D. Slope" : "D", juce::dontSendNotification);
    };

    releaseSlider.onCurveChanged = [this](float val) {
        visualizer.setParams(visualizer.getAttack(), visualizer.getDecay(), 
                             visualizer.getSustain(), visualizer.getRelease(),
                             visualizer.getAttackCurve(), visualizer.getDecayCurve(), val,
                             1.0f, visualizer.getSustainTime());
        if (visualizer.onReleaseCurveChange) visualizer.onReleaseCurveChange(val);
    };
    releaseSlider.onSlopeModeChanged = [this](bool active) {
        releaseLabel.setText(active ? "R. Slope" : "R", juce::dontSendNotification);
    };

    // Sync visualizer backwards if user drags the graph nodes (Using setTimeValue to avoid range clipping)
    visualizer.onAttackChange = [this](float v) { attackSlider.setTimeValue(v); if(onADSRChanged) onADSRChanged(v, (float)decaySlider.getTimeValue(), (float)sustainSlider.getTimeValue(), (float)releaseSlider.getTimeValue(), (float)sustainTimeSlider.getTimeValue()); };
    visualizer.onDecayChange = [this](float v) { decaySlider.setTimeValue(v); if(onADSRChanged) onADSRChanged((float)attackSlider.getTimeValue(), v, (float)sustainSlider.getTimeValue(), (float)releaseSlider.getTimeValue(), (float)sustainTimeSlider.getTimeValue()); };
    visualizer.onSustainChange = [this](float v) { sustainSlider.setTimeValue(v); if(onADSRChanged) onADSRChanged((float)attackSlider.getTimeValue(), (float)decaySlider.getTimeValue(), v, (float)releaseSlider.getTimeValue(), (float)sustainTimeSlider.getTimeValue()); };
    visualizer.onReleaseChange = [this](float v) { releaseSlider.setTimeValue(v); if(onADSRChanged) onADSRChanged((float)attackSlider.getTimeValue(), (float)decaySlider.getTimeValue(), (float)sustainSlider.getTimeValue(), v, (float)sustainTimeSlider.getTimeValue()); };
    visualizer.onSustainTimeChange = [this](float v) { sustainTimeSlider.setTimeValue(v); if(onADSRChanged) onADSRChanged((float)attackSlider.getTimeValue(), (float)decaySlider.getTimeValue(), (float)sustainSlider.getTimeValue(), (float)releaseSlider.getTimeValue(), v); };

    // Curve bidirectional sync - forceSlopeMode FIRST so setCurveValue can update the display
    visualizer.onAttackCurveChange = [this](float v) { 
        attackSlider.forceSlopeMode(true);
        attackSlider.setCurveValue(v); 
    };
    visualizer.onDecayCurveChange = [this](float v) { 
        decaySlider.forceSlopeMode(true);
        decaySlider.setCurveValue(v); 
    };
    visualizer.onReleaseCurveChange = [this](float v) { 
        releaseSlider.forceSlopeMode(true);
        releaseSlider.setCurveValue(v); 
    };
}

void ADSRWidget::setParams(float a, float d, float s, float r, 
                           float aC, float dC, float rC, 
                           float peak, float sTime) {
    attackSlider.setTimeValue(a);
    decaySlider.setTimeValue(d);
    sustainSlider.setTimeValue(s);
    releaseSlider.setTimeValue(r);
    sustainTimeSlider.setTimeValue(sTime);
    
    attackSlider.setCurveValue(aC);
    decaySlider.setCurveValue(dC);
    releaseSlider.setCurveValue(rC);

    visualizer.setParams(a, d, s, r, aC, dC, rC, peak, sTime);
    repaint();
}

void ADSRWidget::updateSliders(float a, float d, float s, float r) {
    attackSlider.setTimeValue(a);
    decaySlider.setTimeValue(d);
    sustainSlider.setTimeValue(s);
    releaseSlider.setTimeValue(r);
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
    float w = (float)sliderArea.getWidth() / 5.0f;
    
    auto aArea = sliderArea.removeFromLeft((int)w);
    attackLabel.setBounds(aArea.removeFromBottom(15));
    attackSlider.setBounds(aArea);
    
    auto dArea = sliderArea.removeFromLeft((int)w);
    decayLabel.setBounds(dArea.removeFromBottom(15));
    decaySlider.setBounds(dArea);
    
    auto sArea = sliderArea.removeFromLeft((int)w);
    sustainLabel.setBounds(sArea.removeFromBottom(15));
    sustainSlider.setBounds(sArea);

    auto stArea = sliderArea.removeFromLeft((int)w);
    sustainTimeLabel.setBounds(stArea.removeFromBottom(15));
    sustainTimeSlider.setBounds(stArea);
    
    auto rlArea = sliderArea;
    releaseLabel.setBounds(rlArea.removeFromBottom(15));
    releaseSlider.setBounds(rlArea);
    
    // Remaining area for the visualizer
    visualizer.setBounds(r.reduced(5));
}

} // namespace sotero
