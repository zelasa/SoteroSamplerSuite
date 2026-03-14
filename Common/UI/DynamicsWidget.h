#pragma once

#include "../SoteroUI.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace sotero {

/**
 * @class DynamicsWidget
 * @brief An atomic widget for compressor controls and monitoring.
 */
class DynamicsWidget : public juce::Component {
public:
    DynamicsWidget();
    ~DynamicsWidget() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Callbacks/Accessors for the parent to wire to APVTS/Engine
    juce::ComboBox& getModeSelector() { return compMode; }
    juce::Slider& getThresholdSlider() { return compThresh; }
    juce::Slider& getRatioSlider() { return compRatio; }
    juce::Slider& getAttackSlider() { return compAttack; }
    juce::Slider& getReleaseSlider() { return compRelease; }
    
    // For visual feedback
    void setReductionLevel(float level) { grMeter.setLevel(level); }

private:
    juce::Label titleLabel{"Dynamics", "COMPRESSOR"};
    juce::ComboBox compMode;
    juce::Slider compThresh, compRatio, compAttack, compRelease;
    juce::Label threshLabel, ratioLabel, attackLabel, releaseLabel;
    
    VUMeter grMeter;
    juce::Label grLabel{"GR", "GR"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicsWidget)
};

} // namespace sotero
