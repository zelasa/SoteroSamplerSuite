#pragma once
#include "../../Common/SoteroFormat.h"
#include <JuceHeader.h>
#include <functional>


namespace sotero {

class SampleRegion : public juce::Component {
public:
  SampleRegion(const KeyMapping &mapping, int noteIndex, int micLayer, int mappingIndex);
  ~SampleRegion() override = default;

  int getMappingIndex() const noexcept { return mappingIndex; }

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
  bool isDragging() const { return currentDragMode != DragMode::None; }
  bool getIsDetachedDrag() const { return isDetachedDrag; }

  void updateFromMapping(const KeyMapping &m) {
    currentMapping = m;
    repaint();
  }
  const KeyMapping &getMapping() const { return currentMapping; }

  enum class DragMode { None, TopHandle, BottomHandle, Body, Eraser };
  DragMode getCurrentDragMode() const { return currentDragMode; }

  std::function<void(const KeyMapping &)> onBoundsChanged;
  std::function<void(const KeyMapping &)> onDragFinished;
  std::function<void(const KeyMapping &)> onClear;
  std::function<void()> onErase;
  std::function<void(const KeyMapping &)> onAudition;
  std::function<void(const KeyMapping &)> onAuditionEnd;
  std::function<void()> onSelect;
  std::function<void(int, bool)> onRequestMove; // delta notes, isAltDown (horizontal drag)
  std::function<void(bool, bool)> onDragStart;
  std::function<void(int, int)> onVerticalSwapRequest; // (deltaVelCenter, mouseScreenY)
  void resetSwapState(int newMouseScreenY); // called by MainComponent after each swap
  bool getIsAltDrag() const noexcept { return isAltDrag; } // MainComponent checks for edge-binding bypass

  int mappingIndex = -1;
  KeyMapping currentMapping;
  int parentNoteIndex;
  int micLayer = 0;

  bool isHovering = false;
  bool isActive = false;
  DragMode currentDragMode = DragMode::None;
  int dragStartY = 0;
  int initialVelHigh = 127;
  int initialVelLow = 0;
  float normalizedGrip = 0.5f; // Relative Y position in range [0.0 (Bottom), 1.0 (Top)]
  bool isDetachedDrag = false;
  bool isAltDrag = false;
  double initialScreenX = 0.0;   // screen X at mouseDown, for absolute horizontal tracking
  double initialScreenY = 0.0;   // screen Y at mouseDown, for absolute tracking
  double initialCenterVel = 64.0; // centre of velocity range at drag start
  bool swapDispatched = false; // guard: fire swap only once per threshold crossing
  int lastNoteOffset = 0;   // accumulated column offset since drag start

  juce::Rectangle<int> getTopHandleBounds() const;
  juce::Rectangle<int> getBottomHandleBounds() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleRegion)
};

} // namespace sotero
