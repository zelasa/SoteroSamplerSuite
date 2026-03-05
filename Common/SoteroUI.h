#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sotero {

/**
    A high-performance VU Meter component with dB scale, smoothing,
    and peak hold functionality.
*/
class VUMeter : public juce::Component {
public:
  VUMeter() {}

  void setLevel(float value) {
    auto db = juce::Decibels::gainToDecibels(value, -60.0f);
    auto targetLevel = juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f);
    targetLevel = juce::jlimit(0.0f, 1.0f, targetLevel);

    if (targetLevel > level) {
      level = targetLevel;
    } else if (targetLevel < 0.001f) {
      level = 0.0f;
    } else {
      level = level * 0.7f + targetLevel * 0.3f;
    }

    if (targetLevel > peakHoldLevel)
      peakHoldLevel = targetLevel;
    else
      peakHoldLevel -= 0.005f;

    peakHoldLevel = juce::jlimit(0.0f, 1.0f, peakHoldLevel);

    if (db > maxDB)
      maxDB = db;

    repaint();
  }

  void paint(juce::Graphics &g) override {
    auto bounds = getLocalBounds().toFloat();
    auto textArea = bounds.removeFromTop(20.0f);

    float barWidth = 14.0f;
    auto barArea =
        bounds.withSizeKeepingCentre(barWidth, bounds.getHeight() - 5.0f)
            .translated(5.0f, 0);

    if (maxDB > 0.1f) {
      g.setColour(juce::Colours::red.withAlpha(0.8f));
      g.fillRoundedRectangle(textArea.reduced(2.0f, 1.0f), 3.0f);
      g.setColour(juce::Colours::white);
    } else {
      g.setColour(juce::Colours::white.withAlpha(0.9f));
    }

    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.drawText(juce::String(maxDB, 1), textArea, juce::Justification::centred);

    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(juce::FontOptions(8.0f));

    float dbTicks[] = {6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -48.0f};
    for (auto tick : dbTicks) {
      auto y =
          juce::jmap(tick, -60.0f, 6.0f, barArea.getBottom(), barArea.getY());
      g.drawText(juce::String(tick, 0), 0, (int)y - 5, (int)barArea.getX() - 4,
                 10, juce::Justification::right);
      g.fillRect(barArea.getX() - 3.0f, y, 2.0f, 1.0f);
    }

    if (level > 0.001f) {
      auto fillHeight = level * barArea.getHeight();
      auto fillBounds = barArea.withTrimmedTop(barArea.getHeight() - fillHeight)
                            .reduced(1.0f);

      auto gradient =
          juce::ColourGradient(juce::Colours::green, 0, barArea.getBottom(),
                               juce::Colours::red, 0, barArea.getY(), false);

      float yellowPos = juce::jmap(-12.0f, -60.0f, 6.0f, 0.0f, 1.0f);
      float redPos = juce::jmap(0.1f, -60.0f, 6.0f, 0.0f, 1.0f);

      gradient.addColour(yellowPos, juce::Colours::yellow);
      gradient.addColour(redPos, juce::Colours::red);

      g.setGradientFill(gradient);
      g.fillRect(fillBounds);
    }

    if (peakHoldLevel > 0.001f) {
      auto peakY = juce::jmap(peakHoldLevel, 0.0f, 1.0f, barArea.getBottom(),
                              barArea.getY());
      float clipThresholdPos = juce::jmap(0.1f, -60.0f, 6.0f, 0.0f, 1.0f);

      g.setColour(peakHoldLevel > (clipThresholdPos)
                      ? juce::Colours::white
                      : juce::Colours::yellow.withAlpha(0.8f));
      g.fillRect(barArea.getX(), peakY, barArea.getWidth(), 2.0f);
    }
  }

  void mouseDown(const juce::MouseEvent &event) override { resetPeak(); }

  void resetPeak() {
    peakHoldLevel = 0.0f;
    maxDB = -60.0f;
    repaint();
  }

  void clear() {
    level = 0.0f;
    peakHoldLevel = 0.0f;
    maxDB = -60.0f;
    repaint();
  }

private:
  float level = 0.0f;
  float peakHoldLevel = 0.0f;
  float maxDB = -60.0f;
};

// --- Custom Buttons ---

class CurveButton : public juce::Button {
public:
  CurveButton(const juce::String &name, int type)
      : juce::Button(name), curveType(type) {
    setClickingTogglesState(true);
    setRadioGroupId(1001);
  }

  void paintButton(juce::Graphics &g, bool, bool) override {
    auto bounds = getLocalBounds().reduced(2);
    auto isSelected = getToggleState();
    g.setColour(isSelected ? juce::Colours::white.withAlpha(0.2f)
                           : juce::Colours::transparentBlack);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    g.setColour(isSelected ? juce::Colours::yellow : juce::Colours::grey);
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, isSelected ? 2.0f : 1.0f);

    auto contentArea = bounds.reduced(5);
    auto iconArea = contentArea.removeFromTop(contentArea.getHeight() * 0.6f);
    g.setColour(isSelected ? juce::Colours::yellow : juce::Colours::white);
    juce::Path p;
    auto pArea = iconArea.toFloat().reduced(4);
    p.startNewSubPath(pArea.getX(), pArea.getBottom());
    if (curveType == 0)
      p.quadraticTo(pArea.getX(), pArea.getY(), pArea.getRight(), pArea.getY());
    else if (curveType == 1)
      p.lineTo(pArea.getRight(), pArea.getY());
    else
      p.quadraticTo(pArea.getRight(), pArea.getBottom(), pArea.getRight(),
                    pArea.getY());
    g.strokePath(p, juce::PathStrokeType(2.5f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(getName(), contentArea, juce::Justification::centred);
  }

private:
  int curveType;
};

class ModeButton : public juce::Button {
public:
  ModeButton(const juce::String &name, int id)
      : juce::Button(name), groupID(id) {
    setClickingTogglesState(true);
    setRadioGroupId(id);
  }

  void paintButton(juce::Graphics &g, bool, bool) override {
    auto bounds = getLocalBounds().reduced(2);
    auto isSelected = getToggleState();
    g.setColour(isSelected ? juce::Colours::orange.withAlpha(0.3f)
                           : juce::Colours::black.withAlpha(0.2f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    g.setColour(isSelected ? juce::Colours::orange : juce::Colours::grey);
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);
    g.setColour(isSelected ? juce::Colours::white : juce::Colours::lightgrey);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(getName(), bounds, juce::Justification::centred);
  }

private:
  int groupID;
};

// --- Customized Knob ---

class HardwareKnob : public juce::Component {
public:
  HardwareKnob(const juce::String &labelText) {
    addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 22);
    slider.setColour(juce::Slider::rotarySliderFillColourId,
                     juce::Colours::orange);
    slider.setColour(juce::Slider::textBoxOutlineColourId,
                     juce::Colours::transparentBlack);
  }

  void resized() override {
    auto area = getLocalBounds();
    label.setBounds(area.removeFromTop(22));
    slider.setBounds(area);
  }

  juce::Slider &getSlider() { return slider; }

private:
  juce::Label label;
  juce::Slider slider;
};

/**
    Minimalist arrow button for navigation.
*/
class SoteroArrowButton : public juce::Button, private juce::Timer {
public:
  SoteroArrowButton(bool pointsLeft)
      : juce::Button(pointsLeft ? "left" : "right"), isLeft(pointsLeft) {
    setTriggeredOnMouseDown(true);
  }

  void mouseDown(const juce::MouseEvent &e) override {
    juce::Button::mouseDown(e);
    startTimer(350); // Initial delay before repeating
  }

  void mouseUp(const juce::MouseEvent &e) override {
    juce::Button::mouseUp(e);
    stopTimer();
  }

  void mouseExit(const juce::MouseEvent &e) override {
    juce::Button::mouseExit(e);
    stopTimer();
  }

  void timerCallback() override {
    startTimer(80); // Speed up repeats
    triggerClick();
  }

  void paintButton(juce::Graphics &g, bool isMouseOver,
                   bool isButtonDown) override {
    auto bounds = getLocalBounds().toFloat();

    if (isMouseOver) {
      g.setColour(juce::Colours::white.withAlpha(0.05f));
      g.fillRect(bounds);
    }

    g.setColour(isButtonDown ? juce::Colours::yellow
                             : juce::Colours::white.withAlpha(0.6f));

    juce::Path p;
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float arrowW = 6.0f;  // Thinner as requested
    float arrowH = 14.0f; // Aspect ratio for elegance

    float x = (w - arrowW) / 2.0f;
    float y = (h - arrowH) / 2.0f;

    if (isLeft) {
      p.startNewSubPath(x + arrowW, y);
      p.lineTo(x, y + arrowH / 2.0f);
      p.lineTo(x + arrowW, y + arrowH);
    } else {
      p.startNewSubPath(x, y);
      p.lineTo(x + arrowW, y + arrowH / 2.0f);
      p.lineTo(x, y + arrowH);
    }

    g.strokePath(p, juce::PathStrokeType(
                        1.5f, juce::PathStrokeType::mitered,
                        juce::PathStrokeType::rounded)); // Thinner stroke
  }

private:
  bool isLeft;
};

} // namespace sotero
