#include "SoteroLookAndFeel.h"
#include "DynamicsWidget.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace sotero {

/**
 * @class AdvancedWidget
 * @brief Widget for Dynamics (Comp), Reverb, and Global Tone.
 */
class AdvancedWidget : public juce::Component {
public:
    AdvancedWidget() {
        addAndMakeVisible(dynamicsWidget);

        addAndMakeVisible(revGroup);
        addAndMakeVisible(revEnable);
        addAndMakeVisible(revSize);
        addAndMakeVisible(revMix);

        addAndMakeVisible(toneGroup);
        addAndMakeVisible(toneSlider);
        addAndMakeVisible(toneLabel);

        revGroup.setText("REVERB");
        revGroup.setColour(juce::GroupComponent::textColourId, SoteroLookAndFeel::getYellowAccent());
        
        toneGroup.setText("GLOBAL TONE");
        toneGroup.setColour(juce::GroupComponent::textColourId, SoteroLookAndFeel::getYellowAccent());
        
        toneLabel.setText("TONE", juce::dontSendNotification);

        auto setupSlider = [](juce::Slider &s, juce::String suffix) {
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
            s.setTextValueSuffix(suffix);
        };

        setupSlider(revSize, " %");
        setupSlider(revMix, " %");
        setupSlider(toneSlider, "");
    }

    void resized() override {
        auto r = getLocalBounds().reduced(10);
        
        auto compArea = r.removeFromLeft(r.getWidth() * 0.55f).reduced(5);
        dynamicsWidget.setBounds(compArea);

        auto revArea = r.removeFromLeft(r.getWidth() * 0.7f).reduced(5);
        revGroup.setBounds(revArea);
        auto rv = revArea.reduced(5, 20);
        revEnable.setBounds(rv.removeFromTop(20));
        float rw = rv.getWidth() / 2.0f;
        revSize.setBounds(rv.removeFromLeft(rw));
        revMix.setBounds(rv);

        auto toneArea = r.reduced(5);
        toneGroup.setBounds(toneArea.reduced(2));
        toneLabel.setBounds(toneArea.removeFromTop(20));
        toneSlider.setBounds(toneArea);
    }

    void paint(juce::Graphics &g) override {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(SoteroLookAndFeel::getWidgetBackground());
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
    }

    DynamicsWidget& getDynamics() { return dynamicsWidget; }
    juce::ToggleButton& getRevEnable() { return revEnable; }
    juce::Slider& getRevSize() { return revSize; }
    juce::Slider& getRevMix() { return revMix; }
    juce::Slider& getToneSlider() { return toneSlider; }

private:
    DynamicsWidget dynamicsWidget;
    juce::GroupComponent revGroup;
    juce::Slider revSize, revMix;
    juce::ToggleButton revEnable{"ON/OFF"};
    juce::GroupComponent toneGroup;
    juce::Slider toneSlider;
    juce::Label toneLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedWidget)
};

} // namespace sotero
