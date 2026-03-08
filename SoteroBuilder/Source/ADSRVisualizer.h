#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace sotero {

/**
 * @class ADSRVisualizer
 * @brief A professional ADSR envelope editor with draggable handles for nodes
 * and curves (slopes).
 */
class ADSRVisualizer : public juce::Component,
                       public juce::SettableTooltipClient {
public:
  ADSRVisualizer() {
    setTooltip("Drag nodes to edit ADSR; Drag segments to edit Slopes");
  }

  // Callbacks for when parameters change
  std::function<void(float)> onAttackChange;
  std::function<void(float)> onDecayChange;
  std::function<void(float)> onSustainChange;
  std::function<void(float)> onReleaseChange;
  std::function<void(float)> onPeakLevelChange;
  std::function<void(float)> onAttackCurveChange;
  std::function<void(float)> onDecayCurveChange;
  std::function<void(float)> onReleaseCurveChange;

  void setParams(float a, float d, float s, float r, float aC = 0.0f,
                 float dC = 0.0f, float rC = 0.0f, float peak = 1.0f) {
    if (draggingIndex != -1)
      return; // Don't interrupt drag

    attack = a;
    decay = d;
    sustain = s;
    release = r;
    attackCurve = aC;
    decayCurve = dC;
    releaseCurve = rC;
    peakLevel = peak;
    repaint();
  }

  void paint(juce::Graphics &g) override {
    auto bounds = getLocalBounds().toFloat();
    auto r = bounds.reduced(30.0f, 20.0f);

    // Background
    g.setColour(juce::Colour(0xff121212));
    g.fillRoundedRectangle(bounds, 4.0f);

    // --- Draw Grid ---
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    const int numSteps = 10;
    for (int i = 1; i < numSteps; ++i) {
      float x = r.getX() + r.getWidth() * (float)i / numSteps;
      g.drawVerticalLine((int)x, r.getY(), r.getBottom());
      float y = r.getY() + r.getHeight() * (float)i / numSteps;
      g.drawHorizontalLine((int)y, r.getX(), r.getRight());
    }

    // --- Calculate Path ---
    auto nodes = getNodes(r);
    juce::Path path;
    path.startNewSubPath(nodes[0]);

    // Attack (Curved)
    drawCurvedSegment(path, nodes[0], nodes[1], attackCurve);
    // Decay (Curved)
    drawCurvedSegment(path, nodes[1], nodes[2], decayCurve);
    // Sustain (Linear)
    path.lineTo(nodes[3]);
    // Release (Curved)
    drawCurvedSegment(path, nodes[3], nodes[4], releaseCurve);

    // --- Draw Fill & Stroke ---
    float opacity = isEnabled() ? 1.0f : 0.2f;
    g.setColour(juce::Colours::yellow.withAlpha(0.1f * opacity));
    juce::Path fillP = path;
    fillP.lineTo(nodes[4].getX(), r.getBottom());
    fillP.lineTo(nodes[0].getX(), r.getBottom());
    fillP.closeSubPath();
    g.fillPath(fillP);

    g.setColour(juce::Colours::yellow.withAlpha(opacity));
    g.strokePath(path, juce::PathStrokeType(2.0f));

    // --- Draw Handles ---
    if (isEnabled()) {
      auto allHandles = getAllPotentialHandles(r);
      for (int i = 0; i < (int)allHandles.size(); ++i) {
        auto hp = allHandles[i];
        bool isCorner = i < 5;
        bool isOver = (i == draggingIndex || i == hoverIndex);

        if (isCorner) {
          // Main Nodes
          if (i == 0)
            continue; // Don't draw start node
          g.setColour(isOver ? juce::Colours::cyan : juce::Colours::white);
          g.fillEllipse(hp.getX() - 4.0f, hp.getY() - 4.0f, 8.0f, 8.0f);
        } else {
          // Slope Nodes (Special styling)
          g.setColour(isOver ? juce::Colours::cyan.withAlpha(0.8f)
                             : juce::Colours::white.withAlpha(0.4f));
          g.drawEllipse(hp.getX() - 3.0f, hp.getY() - 3.0f, 6.0f, 6.0f, 1.0f);
        }
      }
    } else {
      g.setColour(juce::Colours::white.withAlpha(0.4f));
      g.setFont(12.0f);
      g.drawFittedText("SELECT A SAMPLE TO EDIT ADSR", getLocalBounds(),
                       juce::Justification::centred, 1);
    }

    // --- Draw Time Labels ---
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(10.0f);
    for (int i = 0; i <= 5; ++i) {
      float x = r.getX() + (float)i * (r.getWidth() / 5.0f);
      g.drawFittedText(
          juce::String(i * 1000) + "ms",
          juce::Rectangle<int>((int)x - 20, (int)r.getBottom() + 2, 40, 15),
          juce::Justification::centredTop, 1);
    }

    // --- Draw Playhead ---
    if (playheadTime >= 0.0f) {
      float px = r.getX() + (playheadTime / maxT) * r.getWidth();
      if (px <= r.getRight()) {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawVerticalLine((int)px, r.getY(), r.getBottom());

        // Find Y on path
        float py = r.getBottom();
        // Simple search for Y at X
        for (float t = 0; t <= 1.0f; t += 0.01f) {
          auto p = path.getPointAlongPath(t * path.getLength());
          if (std::abs(p.getX() - px) < 2.0f) {
            py = p.getY();
            break;
          }
        }
        g.setColour(juce::Colours::white);
        g.fillEllipse(px - 3.0f, py - 3.0f, 6.0f, 6.0f);
      }
    }
  }

  void setPlayheadTime(float t) {
    if (std::abs(playheadTime - t) > 0.001f) {
      playheadTime = t;
      repaint();
    }
  }

  void mouseMove(const juce::MouseEvent &e) override {
    updateHover(e.position);
  }

  void mouseDown(const juce::MouseEvent &e) override {
    auto r = getLocalBounds().reduced(30, 20).toFloat();
    auto handles = getAllPotentialHandles(r);

    draggingIndex = -1;
    for (int i = 0; i < (int)handles.size(); ++i) {
      if (e.position.getDistanceSquaredFrom(handles[i]) < 100.0f) {
        draggingIndex = i;
        dragStartPos = e.position;
        dragStartValues = {attack,       decay,       sustain,
                           release,      attackCurve, decayCurve,
                           releaseCurve, peakLevel,   sustainTime};
        break;
      }
    }
    repaint();
  }

  void mouseDrag(const juce::MouseEvent &e) override {
    if (draggingIndex == -1)
      return;

    auto r = getLocalBounds().reduced(30, 20).toFloat();
    float dx = (e.position.getX() - dragStartPos.getX()) / r.getWidth() * maxT;
    float dy = (dragStartPos.getY() - e.position.getY()) / r.getHeight();

    // Mapping logic
    if (draggingIndex == 1) { // Attack Node
      attack = juce::jlimit(0.001f, maxT, dragStartValues.a + dx);
      peakLevel = juce::jlimit(0.0f, 1.0f, dragStartValues.peak + dy);
      if (onAttackChange)
        onAttackChange(attack);
      if (onPeakLevelChange)
        onPeakLevelChange(peakLevel);
    } else if (draggingIndex == 2) { // Decay Node
      decay = juce::jlimit(0.001f, maxT, dragStartValues.d + dx);
      sustain = juce::jlimit(0.0f, 1.0f, dragStartValues.s + dy);
      if (onDecayChange)
        onDecayChange(decay);
      if (onSustainChange)
        onSustainChange(sustain);
    } else if (draggingIndex == 3) { // Sustain End Node
      sustainTime = juce::jlimit(0.1f, maxT, dragStartValues.sTime + dx);
      sustain = juce::jlimit(0.0f, 1.0f, dragStartValues.s + dy);
      if (onSustainChange)
        onSustainChange(sustain);
    } else if (draggingIndex == 4) { // Release Node
      release = juce::jlimit(0.001f, maxT, dragStartValues.r + dx);
      if (onReleaseChange)
        onReleaseChange(release);
    } else if (draggingIndex == 5) { // Attack Slope
      attackCurve = juce::jlimit(-1.0f, 1.0f, dragStartValues.aC + dy * 2.0f);
      if (onAttackCurveChange)
        onAttackCurveChange(attackCurve);
    } else if (draggingIndex == 6) { // Decay Slope
      decayCurve = juce::jlimit(-1.0f, 1.0f, dragStartValues.dC + dy * 2.0f);
      if (onDecayCurveChange)
        onDecayCurveChange(decayCurve);
    } else if (draggingIndex == 7) { // Release Slope
      releaseCurve = juce::jlimit(-1.0f, 1.0f, dragStartValues.rC + dy * 2.0f);
      if (onReleaseCurveChange)
        onReleaseCurveChange(releaseCurve);
    }

    repaint();
  }

  void mouseUp(const juce::MouseEvent &) override {
    draggingIndex = -1;
    repaint();
  }

private:
  void updateHover(juce::Point<float> p) {
    auto r = getLocalBounds().reduced(30, 20).toFloat();
    auto handles = getAllPotentialHandles(r);
    int oldHover = hoverIndex;
    hoverIndex = -1;
    for (int i = 0; i < (int)handles.size(); ++i) {
      if (p.getDistanceSquaredFrom(handles[i]) < 100.0f) {
        hoverIndex = i;
        break;
      }
    }
    if (oldHover != hoverIndex)
      repaint();
    setMouseCursor(hoverIndex != -1 ? juce::MouseCursor::PointingHandCursor
                                    : juce::MouseCursor::NormalCursor);
  }

  std::vector<juce::Point<float>> getNodes(juce::Rectangle<float> r) {
    std::vector<juce::Point<float>> n;
    float x = r.getX();
    // 0: Start
    n.push_back({x, r.getBottom()});
    // 1: Attack End
    x += (attack / maxT) * r.getWidth();
    n.push_back({x, r.getY() + (1.0f - peakLevel) * r.getHeight()});
    // 2: Decay End
    x += (decay / maxT) * r.getWidth();
    n.push_back({x, r.getY() + (1.0f - sustain) * r.getHeight()});
    // 3: Sustain End
    x += (sustainTime / maxT) * r.getWidth();
    n.push_back({x, r.getY() + (1.0f - sustain) * r.getHeight()});
    // 4: Release End
    x += (release / maxT) * r.getWidth();
    n.push_back({x, r.getBottom()});

    return n;
  }

  std::vector<juce::Point<float>>
  getAllPotentialHandles(juce::Rectangle<float> r) {
    auto nodes = getNodes(r);
    std::vector<juce::Point<float>> h = nodes;
    // 5: Attack Mid (Slope)
    h.push_back(getPointOnCurve(nodes[0], nodes[1], attackCurve, 0.5f));
    // 6: Decay Mid (Slope)
    h.push_back(getPointOnCurve(nodes[1], nodes[2], decayCurve, 0.5f));
    // 7: Release Mid (Slope)
    h.push_back(getPointOnCurve(nodes[3], nodes[4], releaseCurve, 0.5f));
    return h;
  }

  void drawCurvedSegment(juce::Path &p, juce::Point<float> p1,
                         juce::Point<float> p2, float curve) {
    const int resolution = 20;
    for (int i = 1; i <= resolution; ++i) {
      p.lineTo(getPointOnCurve(p1, p2, curve, (float)i / resolution));
    }
  }

  juce::Point<float> getPointOnCurve(juce::Point<float> p1,
                                     juce::Point<float> p2, float curve,
                                     float t) {
    float k = curve * 5.0f;
    float ct = (std::abs(k) < 0.001f)
                   ? t
                   : (std::exp(k * t) - 1.0f) / (std::exp(k) - 1.0f);
    return {p1.getX() + (p2.getX() - p1.getX()) * t,
            p1.getY() + (p2.getY() - p1.getY()) * ct};
  }

  float attack = 0.1f, decay = 0.1f, sustain = 1.0f, release = 0.2f;
  float attackCurve = 0.0f, decayCurve = 0.0f, releaseCurve = 0.0f;
  float peakLevel = 1.0f;
  float sustainTime = 0.5f; // Visual duration
  const float maxT = 5.0f;

  float playheadTime = -1.0f;
  int draggingIndex = -1;
  int hoverIndex = -1;
  juce::Point<float> dragStartPos;
  struct State {
    float a, d, s, r, aC, dC, rC, peak, sTime;
  } dragStartValues;
};

} // namespace sotero
