#include "FilterWidget.h"

namespace sotero {

FilterWidget::FilterWidget() {
    addAndMakeVisible(titleLabel);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::orange);

    addAndMakeVisible(typeSelector);
    typeSelector.addItem("OFF", 1);
    typeSelector.addItem("LP", 2);
    typeSelector.addItem("HP", 3);
    typeSelector.addItem("BP", 4);
    typeSelector.setSelectedId(1, juce::dontSendNotification);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(10.0f);
        addAndMakeVisible(l);

        s.onValueChange = [this] {
            if (onFilterChanged)
                onFilterChanged(typeSelector.getSelectedId() - 1,
                                (float)cutoffSlider.getValue(),
                                (float)resSlider.getValue());
        };
    };

    setupSlider(cutoffSlider, cutoffLabel, "CUTOFF");
    setupSlider(resSlider, resLabel, "RES");

    cutoffSlider.setRange(20.0, 20000.0, 1.0);
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);
    resSlider.setRange(0.1, 1.0, 0.01);

    typeSelector.onChange = [this] {
        if (onFilterChanged)
            onFilterChanged(typeSelector.getSelectedId() - 1,
                            (float)cutoffSlider.getValue(),
                            (float)resSlider.getValue());
    };
}

void FilterWidget::setParams(int type, float cutoff, float resonance) {
    typeSelector.setSelectedId(type + 1, juce::dontSendNotification);
    cutoffSlider.setValue(cutoff, juce::dontSendNotification);
    resSlider.setValue(resonance, juce::dontSendNotification);
}

void FilterWidget::paint(juce::Graphics& g) {
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);
}

void FilterWidget::resized() {
    auto r = getLocalBounds().reduced(5);
    titleLabel.setBounds(r.removeFromTop(20));
    
    auto topRow = r.removeFromTop(25);
    typeSelector.setBounds(topRow.reduced(2));
    
    float w = (float)r.getWidth() / 2.0f;
    auto cArea = r.removeFromLeft((int)w);
    cutoffLabel.setBounds(cArea.removeFromBottom(15));
    cutoffSlider.setBounds(cArea);
    
    auto reArea = r;
    resLabel.setBounds(reArea.removeFromBottom(15));
    resSlider.setBounds(reArea);
}

} // namespace sotero
