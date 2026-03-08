#pragma once

#include <JuceHeader.h>

namespace sotero {

/**
 * @class ADSRVisualizer
 * @brief An interactive ADSR envelope editor with draggable handles.
 */
class ADSRVisualizer : public juce::Component,
                       public juce::SettableTooltipClient {
public:
  ADSRVisualizer() { setTooltip("Drag handles to edit ADSR envelope"); }

  // Callbacks for when parameters change via dragging
  std::function<void(float)> onAttackChange;
  std::function<void(float)> onDecayChange;
  std::function<void(float)> onSustainChange;
  std::function<void(float)> onReleaseChange;
  std::function<void(float)> onPeakLevelChange;
  std::function<void(float)> onFloorLevelChange;

  void setParams(float a, float d, float s, float r, float peak = 1.0f,
                 float floor = 0.0f) {
    attack = a;
    decay = d;
    sustain = s;
    release = r;
    peakLevel = peak;
    floorLevel = floor;
    repaint();
  }

  void paint(juce::Graphics &g) override {
    auto bounds = getLocalBounds().toFloat();
    auto r = bounds.reduced(30.0f, 20.0f); // Leave space for labels

    // Background
    g.setColour(juce::Colour(0xff121212));
    g.fillRoundedRectangle(bounds, 4.0f);

    // --- Draw Grid and Labels ---
    g.setFont(10.0f);

    // Y-Axis (dB)
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    const int numDbSteps = 5;
    const float dbLevels[] = {0.0f, -6.0f, -12.0f, -24.0f, -48.0f};
    for (int i = 0; i < numDbSteps; ++i) {
      float normVal = juce::Decibels::decibelsToGain(dbLevels[i]);
      float y = r.getY() + r.getHeight() * (1.0f - normVal);

      g.drawLine(r.getX(), y, r.getRight(), y, 0.5f);

      g.setColour(juce::Colours::white.withAlpha(0.5f));
      g.drawText(juce::String((int)dbLevels[i]) + "dB", 0, (int)y - 5,
                 (int)r.getX() - 5, 10, juce::Justification::right);
      g.setColour(juce::Colours::white.withAlpha(0.1f));
    }

    // X-Axis (Time ms/s)
    const float maxTimeSecs = 5.0f;
    for (float t = 0; t <= maxTimeSecs; t += 1.0f) {
      float x = r.getX() + (t / maxTimeSecs) * r.getWidth();
      g.drawLine(x, r.getY(), x, r.getBottom(), 0.5f);

      g.setColour(juce::Colours::white.withAlpha(0.5f));
      juce::String timeLabel = (t < 1.0f) ? juce::String((int)(t * 1000)) + "ms"
                                          : juce::String(t, 1) + "s";
      g.drawText(timeLabel, (int)x - 25, (int)r.getBottom() + 2, 50, 15,
                 juce::Justification::centred);
      g.setColour(juce::Colours::white.withAlpha(0.1f));
    }

    // Calculate points
    auto points = getPoints(r);

    // Draw Envelope Path
    juce::Path p;
    p.startNewSubPath(points[0]);
    for (int i = 1; i < (int)points.size(); ++i)
      p.lineTo(points[i]);

    g.setColour(juce::Colours::yellow.withAlpha(0.15f));
    juce::Path fillP = p;
    fillP.lineTo(points.back().getX(), r.getBottom());
    fillP.lineTo(points.front().getX(), r.getBottom());
    fillP.closeSubPath();
    g.fillPath(fillP);

    g.setColour(juce::Colours::yellow);
    g.strokePath(p, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));

    // Draw Handles
    for (int i = 1; i < (int)points.size(); ++i) {
      auto hp = points[i];
      bool isOver = (i == draggingIndex || i == hoverIndex);
      g.setColour(isOver ? juce::Colours::cyan : juce::Colours::white);
      g.fillEllipse(hp.getX() - 4.0f, hp.getY() - 4.0f, 8.0f, 8.0f);

      if (isOver) {
        g.setColour(juce::Colours::cyan.withAlpha(0.3f));
        g.drawEllipse(hp.getX() - 8.0f, hp.getY() - 8.0f, 16.0f, 16.0f, 1.0f);
      }
    }
  }

  void mouseMove(const juce::MouseEvent &e) override {
    auto bounds = getLocalBounds().toFloat();
    auto r = bounds.reduced(30.0f, 20.0f);
    auto points = getPoints(r);

    int oldHover = hoverIndex;
    hoverIndex = -1;

    for (int i = 1; i < points.size(); ++i) {
      if (e.position.getDistanceSquaredFrom(points[i]) < 100.0f) {
        hoverIndex = i;
        break;
      }
    }

    if (oldHover != hoverIndex)
      repaint();

    setMouseCursor(hoverIndex != -1 ? juce::MouseCursor::PointingHandCursor
                                    : juce::MouseCursor::NormalCursor);
  }

  void mouseDown(const juce::MouseEvent &e) override {
    auto bounds = getLocalBounds().toFloat();
    auto r = bounds.reduced(30.0f, 20.0f);
    auto points = getPoints(r);

    draggingIndex = -1;
    for (int i = 1; i < points.size(); ++i) {
      if (e.position.getDistanceSquaredFrom(points[i]) < 100.0f) {
        draggingIndex = i;
        break;
      }
    }
    repaint();
  }

  void mouseDrag(const juce::MouseEvent &e) override {
    if (draggingIndex == -1)
      return;

    auto bounds = getLocalBounds().toFloat();
    auto r = bounds.reduced(30.0f, 20.0f);
    float normX = (e.position.getX() - r.getX()) / r.getWidth();
    float normY = 1.0f - (e.position.getY() - r.getY()) / r.getHeight();

    normX = juce::jlimit(0.0f, 1.0f, normX);
    normY = juce::jlimit(0.0f, 1.0f, normY);

    // Adjust parameters based on which point is dragged
    const float maxPossibleTime = 5.0f;

    if (draggingIndex == 1) { // 1: Attack Peak
      attack = juce::jlimit(0.001f, maxPossibleTime, normX * maxPossibleTime);
      peakLevel = juce::jlimit(0.0f, 1.0f, normY);
      if (onAttackChange)
        onAttackChange(attack);
      if (onPeakLevelChange)
        onPeakLevelChange(peakLevel);
    } else if (draggingIndex == 2) { // 2: Decay / Sustain Level Start
      float attackNorm = juce::jmax(0.001f, attack / maxPossibleTime);
      decay = juce::jlimit(0.001f, maxPossibleTime,
                           (normX - attackNorm) * maxPossibleTime);
      sustain = juce::jlimit(0.0f, 1.0f, normY);
      if (onDecayChange)
        onDecayChange(decay);
      if (onSustainChange)
        onSustainChange(sustain);
    } else if (draggingIndex == 3) { // 3: Sustain End
      // X here adjusts a visual sustainEndNorm (internal or fixed at 0.8)
      // For now let's keep X as a visual stay, but Y affects sustain
      sustain = juce::jlimit(0.0f, 1.0f, normY);
      if (onSustainChange)
        onSustainChange(sustain);
    } else if (draggingIndex == 4) { // 4: Release End
      float releaseStartNorm = 0.8f;
      release = juce::jlimit(0.001f, maxPossibleTime,
                             (normX - releaseStartNorm) * maxPossibleTime);
      floorLevel = juce::jlimit(0.0f, 1.0f, normY);
      if (onReleaseChange)
        onReleaseChange(release);
      if (onFloorLevelChange)
        onFloorLevelChange(floorLevel);
    }

    repaint();
  }

  void mouseUp(const juce::MouseEvent &) override {
    draggingIndex = -1;
    repaint();
  }

private:
  std::vector<juce::Point<float>> getPoints(juce::Rectangle<float> r) {
    std::vector<juce::Point<float>> p;
    const float maxT = 5.0f;

    // 0: Start (Bottom Left)
    p.push_back({r.getX(), r.getBottom()});

    // 1: Attack Peak
    float aX = r.getX() + (attack / maxT) * r.getWidth();
    aX = juce::jlimit(r.getX(), r.getRight(), aX);
    float peakY = r.getY() + (1.0f - peakLevel) * r.getHeight();
    p.push_back({aX, peakY});

    // 2: Decay / Sustain Level
    float dX = aX + (decay / maxT) * r.getWidth();
    dX = juce::jlimit(aX, r.getRight(), dX);
    float sY = r.getY() + (1.0f - sustain) * r.getHeight();
    p.push_back({dX, sY});

    // 3: Sustain End (Fixed visual segment for the "Sustain" phase)
    float sustainEndX = juce::jmax(dX + 10.0f, r.getX() + 0.8f * r.getWidth());
    sustainEndX = juce::jlimit(dX, r.getRight(), sustainEndX);
    p.push_back({sustainEndX, sY});

    // 4: Release End
    float rX = sustainEndX + (release / maxT) * r.getWidth();
    rX = juce::jlimit(sustainEndX, r.getRight(), rX);
    float floorY = r.getY() + (1.0f - floorLevel) * r.getHeight();
    p.push_back({rX, floorY});

    return p;
  }

  float attack = 0.1f;
  float decay = 0.1f;
  float sustain = 0.8f;
  float release = 0.2f;
  float peakLevel = 1.0f;
  float floorLevel = 0.0f;

  int draggingIndex = -1;
  int hoverIndex = -1;
};

} // namespace sotero
