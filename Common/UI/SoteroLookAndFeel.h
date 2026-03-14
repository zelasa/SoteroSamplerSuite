#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace sotero {

/**
 * @class SoteroLookAndFeel
 * @brief Centralized styling for the Sotero Sampler Suite.
 * Implements a modern, premium dark theme with vibrant accents.
 */
class SoteroLookAndFeel : public juce::LookAndFeel_V4 {
public:
    SoteroLookAndFeel() {
        // Base Palette
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff0a0a0a));
        setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        
        // Slider Colours (Default)
        setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan.withAlpha(0.6f));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha(0.1f));
        
        // Button Colours
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.8f));
        setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan.withAlpha(0.4f));

        // ComboBox
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.1f));
    }

    // --- Custom Rotary Design ---
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider& slider) override {
        auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
        auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = 3.0f;
        auto arcRadius = radius - lineW * 0.5f;

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(outline);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled()) {
            juce::Path valueArc;
            valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius,
                                   0.0f, rotaryStartAngle, toAngle, true);
            g.setColour(fill);
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Glowing Dot indicator
        auto dotRadius = 3.0f;
        juce::Point<float> thumbPos(bounds.getCentreX() + arcRadius * std::cos(toAngle - juce::MathConstants<float>::halfPi),
                                    bounds.getCentreY() + arcRadius * std::sin(toAngle - juce::MathConstants<float>::halfPi));
        
        g.setColour(slider.isEnabled() ? fill : juce::Colours::grey);
        g.fillEllipse(juce::Rectangle<float>(dotRadius * 2.0f, dotRadius * 2.0f).withCentre(thumbPos));
    }

    // --- Custom Label Fonts ---
    juce::Font getLabelFont(juce::Label& label) override {
        return juce::Font("Outfit", 14.0f, juce::Font::plain);
    }

    // Custom Button rendering for a "Glass" look
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                           .withMultipliedAlpha(shouldDrawButtonAsDown ? 0.8f : (shouldDrawButtonAsHighlighted ? 1.2f : 1.0f));

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            g.setColour(baseColour.withAlpha(0.2f));
        else
            g.setColour(baseColour.withAlpha(0.1f));
            
        g.fillRoundedRectangle(bounds, 4.0f);
        
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    // Custom Colours for specific regions (Static Helpers)
    static juce::Colour getLayer1Colour() { return juce::Colours::cyan; }
    static juce::Colour getLayer2Colour() { return juce::Colours::red; }
    static juce::Colour getYellowAccent() { return juce::Colours::yellow; }
    static juce::Colour getOrangeAccent() { return juce::Colours::orange; }
    static juce::Colour getWidgetBackground() { return juce::Colour(0xff121212); }
};

} // namespace sotero
