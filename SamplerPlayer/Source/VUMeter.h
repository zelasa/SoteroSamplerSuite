#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
    A high-performance VU Meter component with dB scale, smoothing,
    and peak hold functionality.
*/
class VUMeter : public juce::Component {
public:
  VUMeter() {}

  void setLevel(float value) {
    // 1. Conversão: Transforma ganho linear (0..1) em Decibels (-60dB..+6dB)
    auto db = juce::Decibels::gainToDecibels(value, -60.0f);

    // 2. Mapeamento: Transforma a escala dB no range 0..1 para desenho
    auto targetLevel = juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f);
    targetLevel = juce::jlimit(0.0f, 1.0f, targetLevel);

    // 3. Suavização (Decay): Se o sinal subir, sobe imediato. Se cair, cai
    // suave.
    if (targetLevel > level) {
      level = targetLevel;
    } else if (targetLevel < 0.001f) {
      level = 0.0f;
    } else {
      level = level * 0.7f + targetLevel * 0.3f;
    }

    // Peak Hold logic
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

    // Peak text
    if (maxDB > 0.1f) {
      g.setColour(juce::Colours::red.withAlpha(0.8f));
      g.fillRoundedRectangle(textArea.reduced(2.0f, 1.0f), 3.0f);
      g.setColour(juce::Colours::white);
    } else {
      g.setColour(juce::Colours::white.withAlpha(0.9f));
    }

    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.drawText(juce::String(maxDB, 1), textArea, juce::Justification::centred);

    // Scale
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

    // Audio bar
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

    // Peak Hold marker
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
