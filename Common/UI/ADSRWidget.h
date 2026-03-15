#pragma once

#include "ADSRVisualizer.h"
#include "SoteroLookAndFeel.h"
#include "../SoteroUI.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace sotero {

/**
 * @class ADSRParamSlider
 * @brief Custom slider that handles Ctrl+Drag for curves/slopes.
 */
class ADSRParamSlider : public juce::Slider, private juce::Timer {
public:
    ADSRParamSlider() : juce::Slider() {}
    
    ~ADSRParamSlider() override {
        stopTimer();
    }
    
    std::function<void(float)> onCurveChanged;
    std::function<void(bool)> onSlopeModeChanged;
    
    void setTimeRange(double min, double max, double step) {
        timeMin = min; timeMax = max; timeStep = step;
        // Don't force range if we are currently dragging a slope
        if (!isSlopeMode) setRange(timeMin, timeMax, timeStep);
    }

    void setTimeValue(double newValue) {
        timeValue = newValue;
        if (!isSlopeMode && !isDragging) {
            isUpdatingProgrammatically = true;
            juce::Slider::setValue(timeValue, juce::dontSendNotification);
            isUpdatingProgrammatically = false;
        }
    }

    void setCurveValue(float newValue) {
        curveValue = newValue;
        if (isSlopeMode) {
            isUpdatingProgrammatically = true;
            juce::Slider::setValue(curveValue, juce::dontSendNotification);
            isUpdatingProgrammatically = false;
        }
    }

    float getCurveValue() const { return curveValue; }
    
    float getTimeValue() const { 
        return (float)timeValue;
    }

    void mouseEnter(const juce::MouseEvent& e) override { 
        if (!isDragging) startTimerHz(50); 
        updateModeState(e.mods, false); 
    }
    
    void mouseMove(const juce::MouseEvent& e) override { 
        updateModeState(e.mods, false); 
    }
    
    void mouseExit(const juce::MouseEvent& e) override { 
        if (!isDragging) stopTimer();
        updateModeState(e.mods, true); 
    }

    void mouseDown(const juce::MouseEvent& e) override {
        isDragging = true;
        stopTimer();
        // Force state based on Ctrl *at start* of drag
        isSlopeMode = e.mods.isCtrlDown();
        
        if (isSlopeMode) {
            setRange(-1.0, 1.0, 0.0001);
            setMouseDragSensitivity(150); // High-res, professional feel
            juce::Slider::setValue(curveValue, juce::dontSendNotification);
            setColour(juce::Slider::rotarySliderFillColourId, SoteroLookAndFeel::getOrangeAccent());
            setColour(juce::Slider::thumbColourId, SoteroLookAndFeel::getOrangeAccent());
            if (onSlopeModeChanged) onSlopeModeChanged(true);
        } else {
            setRange(timeMin, timeMax, timeStep);
            setMouseDragSensitivity(250); 
            juce::Slider::setValue(timeValue, juce::dontSendNotification);
            setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan.withAlpha(0.6f));
            setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
            if (onSlopeModeChanged) onSlopeModeChanged(false);
        }

        juce::Slider::mouseDown(e);
    }
    
    void mouseUp(const juce::MouseEvent& e) override {
        juce::Slider::mouseUp(e);
        isDragging = false;
        startTimerHz(50);
        updateModeState(e.mods, !isMouseOver());
    }
    
    void valueChanged() override {
        juce::Slider::valueChanged();
        // Only propagate user interactions, not programmatic updates
        if (isUpdatingProgrammatically) return;
        if (isSlopeMode) {
            curveValue = (float)getValue();
            if (isDragging && onCurveChanged) onCurveChanged(curveValue);
        } else {
            timeValue = getValue();
        }
    }

    void timerCallback() override {
        if (!isDragging) {
            updateModeState(juce::ModifierKeys::getCurrentModifiers(), !isMouseOver());
        }
    }

    // Force mode from outside (called from graph drag callbacks)
    // This BYPASSES the isDragging guard intentionally
    void forceSlopeMode(bool shouldBeSlope) {
        if (shouldBeSlope == isSlopeMode) return;
        isSlopeMode = shouldBeSlope;
        isUpdatingProgrammatically = true;
        if (isSlopeMode) {
            setRange(-1.0, 1.0, 0.0001);
            setMouseDragSensitivity(80);
            juce::Slider::setValue(curveValue, juce::dontSendNotification);
            setColour(juce::Slider::rotarySliderFillColourId, SoteroLookAndFeel::getOrangeAccent());
            setColour(juce::Slider::thumbColourId, SoteroLookAndFeel::getOrangeAccent());
        } else {
            setRange(timeMin, timeMax, timeStep);
            setMouseDragSensitivity(120);
            juce::Slider::setValue(timeValue, juce::dontSendNotification);
            setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan.withAlpha(0.6f));
            setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
        }
        isUpdatingProgrammatically = false;
        if (onSlopeModeChanged) onSlopeModeChanged(isSlopeMode);
    }

    bool isMouseOverOrDragging() const { return isMouseOver() || isDragging; }

private:
    void updateModeState(const juce::ModifierKeys& mods, bool isExitEvent, bool forcedSlope = false) {
        if (isDragging) return;

        bool shouldBeSlope = (mods.isCtrlDown() || forcedSlope) && !isExitEvent;
        
        if (shouldBeSlope != isSlopeMode) {
            isSlopeMode = shouldBeSlope;
            if (isSlopeMode) {
                setRange(-1.0, 1.0, 0.0001);
                setMouseDragSensitivity(80); // Fast, responsive for fine curves
                juce::Slider::setValue(curveValue, juce::dontSendNotification);
                setColour(juce::Slider::rotarySliderFillColourId, SoteroLookAndFeel::getOrangeAccent());
                setColour(juce::Slider::thumbColourId, SoteroLookAndFeel::getOrangeAccent());
            } else {
                setRange(timeMin, timeMax, timeStep);
                setMouseDragSensitivity(120); // Faster than default for better handle
                juce::Slider::setValue(timeValue, juce::dontSendNotification);
                setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan.withAlpha(0.6f));
                setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
            }
            if (onSlopeModeChanged) onSlopeModeChanged(isSlopeMode);
        }
    }

    bool isSlopeMode = false;
    bool isDragging = false;
    bool isUpdatingProgrammatically = false;
    float curveValue = 0.0f;
    double timeValue = 0.0;
    double timeMin = 0.0, timeMax = 1.0, timeStep = 0.001;
};

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
    std::function<void(float, float, float, float, float)> onADSRChanged;

    void setParams(float a, float d, float s, float r, 
                   float aC = 0.0f, float dC = 0.0f, float rC = 0.0f, 
                   float peak = 1.0f, float sTime = 0.5f);

    ADSRVisualizer& getVisualizer() { return visualizer; }

private:
    ADSRVisualizer visualizer;
    ADSRParamSlider attackSlider, decaySlider, sustainSlider, releaseSlider, sustainTimeSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel, sustainTimeLabel;

    void updateSliders(float a, float d, float s, float r);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSRWidget)
};

} // namespace sotero
