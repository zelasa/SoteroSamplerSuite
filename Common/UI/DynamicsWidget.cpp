#include "DynamicsWidget.h"

namespace sotero {

DynamicsWidget::DynamicsWidget() {
    addAndMakeVisible(titleLabel);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::orange);

    addAndMakeVisible(compMode);
    compMode.addItem("OFF", 1);
    compMode.addItem("CLEAN", 2);
    compMode.addItem("WARM", 3);
    compMode.addItem("PUNCH", 4);
    compMode.setSelectedId(1, juce::dontSendNotification);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& suffix) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(9.0f);
        addAndMakeVisible(l);
    };

    setupSlider(compThresh, threshLabel, "THRESH", " dB");
    setupSlider(compRatio, ratioLabel, "RATIO", ":1");
    setupSlider(compAttack, attackLabel, "ATTACK", " ms");
    setupSlider(compRelease, releaseLabel, "RELEASE", " ms");

    compThresh.setRange(-60.0, 0.0, 0.1);
    compRatio.setRange(1.0, 20.0, 0.1);
    compAttack.setRange(0.1, 100.0, 0.1);
    compRelease.setRange(10.0, 1000.0, 1.0);

    addAndMakeVisible(grMeter);
    addAndMakeVisible(grLabel);
    grLabel.setFont(10.0f);
    grLabel.setJustificationType(juce::Justification::centred);
}

void DynamicsWidget::paint(juce::Graphics& g) {
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);
}

void DynamicsWidget::resized() {
    auto r = getLocalBounds().reduced(5);
    titleLabel.setBounds(r.removeFromTop(20));
    
    auto topArea = r.removeFromTop(25);
    compMode.setBounds(topArea.removeFromLeft(100).reduced(2));
    
    // Meter on the right
    auto meterArea = r.removeFromRight(30);
    grLabel.setBounds(meterArea.removeFromTop(15));
    grMeter.setBounds(meterArea.reduced(2));
    
    // Sliders grid
    float w = (float)r.getWidth() / 4.0f;
    auto tArea = r.removeFromLeft((int)w);
    threshLabel.setBounds(tArea.removeFromBottom(15));
    compThresh.setBounds(tArea);
    
    auto raArea = r.removeFromLeft((int)w);
    ratioLabel.setBounds(raArea.removeFromBottom(15));
    compRatio.setBounds(raArea);
    
    auto aArea = r.removeFromLeft((int)w);
    attackLabel.setBounds(aArea.removeFromBottom(15));
    compAttack.setBounds(aArea);
    
    auto reArea = r;
    releaseLabel.setBounds(reArea.removeFromBottom(15));
    compRelease.setBounds(reArea);
}

} // namespace sotero
