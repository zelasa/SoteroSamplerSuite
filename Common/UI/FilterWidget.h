#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace sotero {

/**
 * @class FilterWidget
 * @brief An atomic widget for filter controls (Type, Cutoff, Resonance).
 */
class FilterWidget : public juce::Component {
public:
    FilterWidget();
    ~FilterWidget() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Callbacks
    std::function<void(int, float, float)> onFilterChanged;

    void setParams(int type, float cutoff, float resonance);

private:
    juce::Label titleLabel{"Filter", "FILTER"};
    juce::ComboBox typeSelector;
    juce::Slider cutoffSlider, resSlider;
    juce::Label cutoffLabel, resLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterWidget)
};

} // namespace sotero
