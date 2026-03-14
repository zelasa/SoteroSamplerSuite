#pragma once

#include "ADSRVisualizer.h"
#include "../SoteroUI.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace sotero {

/**
 * @class ADSRWidget
 * @brief An atomic widget combining the ADSR graph and its level controls.
 */
class ADSRWidget : public juce::Component {
public:
    ADSRWidget();
    ~ADSRWidget() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Callbacks for the parent container
    std::function<void(float, float, float, float)> onADSRChanged;

    void setParams(float a, float d, float s, float r, 
                   float aC = 0.0f, float dC = 0.0f, float rC = 0.0f, 
                   float peak = 1.0f, float sTime = 0.5f);

    ADSRVisualizer& getVisualizer() { return visualizer; }

private:
    ADSRVisualizer visualizer;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;

    void updateSliders(float a, float d, float s, float r);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSRWidget)
};

} // namespace sotero
