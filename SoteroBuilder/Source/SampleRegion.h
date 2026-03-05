#pragma once
#include "../../Common/SoteroFormat.h"
#include <JuceHeader.h>

namespace sotero {

class SampleRegion : public juce::Component {
public:
  SampleRegion(const KeyMapping &mapping, int noteIndex);
  ~SampleRegion() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;

  void setActive(bool active) {
    isActive = active;
    repaint();
  }
  bool getActive() const { return isActive; }

  const KeyMapping &getMapping() const { return currentMapping; }

  std::function<void(const KeyMapping &)> onBoundsChanged;
  std::function<void(const KeyMapping &)> onDragFinished;
  std::function<void(const KeyMapping &)> onClear;
  std::function<void(const KeyMapping &)> onAudition;
  std::function<void()> onSelect;

private:
  KeyMapping currentMapping;
  int parentNoteIndex;

  SampleRegion *gluedTopNeighbor = nullptr;
  SampleRegion *gluedBottomNeighbor = nullptr;

  bool isHovering = false;
  bool isActive = false;
  enum class DragMode { None, TopHandle, BottomHandle, Body, Eraser };
  DragMode currentDragMode = DragMode::None;
  int dragStartY = 0;
  int initialVelHigh = 127;
  int initialVelLow = 0;

  juce::Rectangle<int> getTopHandleBounds() const;
  juce::Rectangle<int> getBottomHandleBounds() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleRegion)
};

} // namespace sotero
