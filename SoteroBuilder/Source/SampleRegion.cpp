#include "SampleRegion.h"

namespace sotero {

SampleRegion::SampleRegion(const KeyMapping &mapping, int nIdx, int layer, int mIdx)
    : currentMapping(mapping), parentNoteIndex(nIdx), micLayer(layer), mappingIndex(mIdx) {
  setRepaintsOnMouseActivity(true);
}

void SampleRegion::paint(juce::Graphics &g) {
  if (currentMapping.samplePath.isEmpty())
    return;

  auto bounds = getLocalBounds().toFloat();
  // Ensure visual minimum height for tiny logical ranges
  if (bounds.getHeight() < 6.0f) {
    bounds = bounds.withHeight(6.0f).withCentre(bounds.getCentre());
  }

  // Base color logic
  juce::Colour baseColor;
  if (micLayer == 0) { // Layer A (Blue tones)
    baseColor = isActive ? juce::Colour(0xff0088cc) : juce::Colour(0xff00d2ff);
  } else { // Layer B (Red tones)
    baseColor = isActive ? juce::Colour(0xffaa2222) : juce::Colour(0xffff4d4d);
  }

  if (isActive) {
    // Premium glow for active sample
    auto glow = g.getClipBounds().toFloat();
    juce::Graphics::ScopedSaveState ss(g);
    g.setGradientFill(juce::ColourGradient(
        baseColor.withAlpha(0.8f), bounds.getCentreX(), bounds.getCentreY(),
        baseColor.withAlpha(0.2f), 0, 0, true));
    g.fillRoundedRectangle(bounds.reduced(1.0f), 3.0f);
  }

  g.setColour(
      baseColor.withAlpha(isActive ? 0.85f : (isHovering ? 0.8f : 0.6f)));
  g.fillRoundedRectangle(bounds.reduced(1.0f), 3.0f);

  // Border
  if (isActive) {
    g.setColour(juce::Colours::yellow); // High contrast for selection
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 2.5f);
  } else {
    g.setColour(juce::Colours::white.withAlpha(isHovering ? 0.9f : 0.4f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 3.0f,
                           isHovering ? 1.5f : 1.0f);
  }

  // Handles (Only on Hover)
  if (isHovering) {
    g.setColour(isActive ? juce::Colours::white
                         : juce::Colours::white.withAlpha(0.95f));

    auto topH = getTopHandleBounds()
                    .toFloat()
                    .withSize(28.0f, 5.0f)
                    .withCentre(getTopHandleBounds().getCentre().toFloat());
    auto bottomH =
        getBottomHandleBounds()
            .toFloat()
            .withSize(28.0f, 5.0f)
            .withCentre(getBottomHandleBounds().getCentre().toFloat());

    g.fillRoundedRectangle(topH, 2.0f);
    g.fillRoundedRectangle(bottomH, 2.0f);

    // Optional: add a tiny orange glow if active
    if (isActive) {
      g.setColour(juce::Colours::orange.withAlpha(0.3f));
      g.drawRoundedRectangle(topH.expanded(1.0f), 2.0f, 1.0f);
      g.drawRoundedRectangle(bottomH.expanded(1.0f), 2.0f, 1.0f);
    }
  } else if (isActive) {
    // Keep handles slightly visible when active but not hovered?
    // Or just the main border? Let's use a subtle orange to show they are
    // there.
    g.setColour(juce::Colours::orange.withAlpha(0.6f));
    auto topH = getTopHandleBounds()
                    .toFloat()
                    .withSize(24.0f, 4.0f)
                    .withCentre(getTopHandleBounds().getCentre().toFloat());
    auto bottomH =
        getBottomHandleBounds()
            .toFloat()
            .withSize(24.0f, 4.0f)
            .withCentre(getBottomHandleBounds().getCentre().toFloat());
    g.fillRoundedRectangle(topH, 2.0f);
    g.fillRoundedRectangle(bottomH, 2.0f);
  }

  // Draw Sample Name - avoid crash if path is corrupt
  if (currentMapping.samplePath.isNotEmpty()) {
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    auto fileName = juce::File(currentMapping.samplePath).getFileName();
    g.setFont(10.0f);

    // Calculate a safe area for text (away from handles)
    auto textArea = getLocalBounds().reduced(2, 6);
    if (textArea.getHeight() > 10) {
      g.drawFittedText(fileName, textArea, juce::Justification::centred, 2);
    }
  }

  // Draw Velocity Values
  if (currentDragMode != DragMode::None) {
    g.setColour(juce::Colours::yellow);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    juce::String dragText = juce::String(currentMapping.velocityHigh) + " - " +
                            juce::String(currentMapping.velocityLow);
    g.drawFittedText(dragText, getLocalBounds(), juce::Justification::centred,
                     1);
  } else {
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    if (getLocalBounds().getHeight() < 30) {
      g.setFont(9.0f);
      juce::String velText = juce::String(currentMapping.velocityHigh) + " - " +
                             juce::String(currentMapping.velocityLow);
      g.drawFittedText(velText, getLocalBounds(), juce::Justification::centred,
                       1);
    } else {
      g.setFont(9.0f);
      g.drawText(juce::String(currentMapping.velocityHigh),
                 getLocalBounds().removeFromTop(12).reduced(2, 0),
                 juce::Justification::topRight);
      g.drawText(juce::String(currentMapping.velocityLow),
                 getLocalBounds().removeFromBottom(12).reduced(2, 0),
                 juce::Justification::bottomRight);
    }
  }
}

void SampleRegion::resized() {}

juce::Rectangle<int> SampleRegion::getTopHandleBounds() const {
  int h = getLocalBounds().getHeight();
  int handleH = juce::jlimit(8, 12, h / 4);
  return getLocalBounds().removeFromTop(handleH);
}

juce::Rectangle<int> SampleRegion::getBottomHandleBounds() const {
  int h = getLocalBounds().getHeight();
  int handleH = juce::jlimit(8, 12, h / 4);
  return getLocalBounds().removeFromBottom(handleH);
}

void SampleRegion::mouseEnter(const juce::MouseEvent &e) {
  isHovering = true;
  repaint();
}

void SampleRegion::mouseMove(const juce::MouseEvent &e) {
  if (getTopHandleBounds().contains(e.getPosition()) ||
      getBottomHandleBounds().contains(e.getPosition())) {
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
  } else {
    setMouseCursor(juce::MouseCursor::NormalCursor);
  }
}

void SampleRegion::mouseExit(const juce::MouseEvent &e) {
  isHovering = false;
  setMouseCursor(juce::MouseCursor::NormalCursor);
  repaint();
}

void SampleRegion::mouseDown(const juce::MouseEvent &e) {
  juce::Component::SafePointer<SampleRegion> safeThis(this);
  toFront(true);

  if (onSelect)
    onSelect();
  if (safeThis == nullptr)
    return;

  setActive(true);

  if (e.mods.isRightButtonDown() && e.mods.isShiftDown()) {
    currentDragMode = DragMode::Eraser;
    if (onAudition && (e.mods.isCtrlDown() || e.mods.isCommandDown()))
      onAudition(currentMapping);
    dragStartY = e.getScreenY();
    currentMapping.samplePath = "";
    repaint();
    return;
  }

  if (getTopHandleBounds().contains(e.getPosition())) {
    currentDragMode = DragMode::TopHandle;
  } else if (getBottomHandleBounds().contains(e.getPosition())) {
    currentDragMode = DragMode::BottomHandle;
  } else {
    currentDragMode = DragMode::Body;
    if (onAudition && (e.mods.isCtrlDown() || e.mods.isCommandDown()))
      onAudition(currentMapping);
  }

  // Capture Alt key state and initial velocity center for swap detection
  isAltDrag = e.mods.isAltDown();
  swapDispatched = false;
  // Capture initial screen X for continuous horizontal tracking
  initialScreenX = e.getScreenX();
  lastNoteOffset = 0;

  dragStartY = e.getScreenY();
  initialVelLow  = currentMapping.velocityLow;
  initialVelHigh = currentMapping.velocityHigh;
  initialCenterVel = (initialVelLow + initialVelHigh) / 2;

  bool stickyTop = false;
  bool stickyBottom = false;

  if (auto *parent = getParentComponent()) {
    for (auto *sibling : parent->getChildren()) {
      if (auto *other = dynamic_cast<SampleRegion *>(sibling)) {
        if (other != this && other->currentMapping.samplePath.isNotEmpty()) {
          if (other->currentMapping.velocityLow == currentMapping.velocityHigh + 1)
            stickyTop = true;
          if (other->currentMapping.velocityHigh == currentMapping.velocityLow - 1)
            stickyBottom = true;
        }
      }
    }
  }

  if (onDragStart)
    onDragStart(stickyTop, stickyBottom);
}

void SampleRegion::mouseDrag(const juce::MouseEvent &e) {
  juce::Component::SafePointer<SampleRegion> safeThis(this);
  if (currentDragMode == DragMode::None)
    return;

  // --- Eraser Mode ---
  if (currentDragMode == DragMode::Eraser) {
    if (auto *parent = getParentComponent()) {
      auto screenPos = e.getScreenPosition();
      for (auto *sibling : parent->getChildren()) {
        if (auto *other = dynamic_cast<SampleRegion *>(sibling)) {
          if (other->getScreenBounds().contains(screenPos.toInt())) {
            if (other->currentMapping.samplePath.isNotEmpty()) {
              other->currentMapping.samplePath = "";
              if (other->onErase)
                other->onErase();
              other->repaint();
            }
          }
        }
      }
    }
    return;
  }

  if (e.mods.isRightButtonDown() && !e.mods.isShiftDown())
    return;

  // Re-read Alt key state dynamically every drag tick
  isAltDrag = e.mods.isAltDown();

  if (auto *parent = getParentComponent()) {
    float parentHeight = (float)parent->getHeight();
    if (parentHeight <= 0)
      return;

    float pixelsPerVelocity = parentHeight / 128.0f;
    int dragDeltaY = e.getScreenY() - dragStartY;
    float velocityDeltaFloat = -((float)dragDeltaY / pixelsPerVelocity);
    int velocityDelta = juce::roundToInt(velocityDeltaFloat);

    const int limitLow  = 0;
    const int limitHigh = 127;
    bool boundsChanged = false;

    // -------------------------------------------------------
    // HANDLES (TopHandle / BottomHandle) – always operational 
    // -------------------------------------------------------
    if (currentDragMode == DragMode::TopHandle) {
      int newHighL = currentMapping.velocityLow + 1;
      int newHighH = juce::jmax(newHighL, limitHigh);
      int newHigh  = juce::jlimit(newHighL, newHighH, initialVelHigh + velocityDelta);
      if (currentMapping.velocityHigh != newHigh) {
        currentMapping.velocityHigh = newHigh;
        boundsChanged = true;
      }
    } else if (currentDragMode == DragMode::BottomHandle) {
      int newLowH = currentMapping.velocityHigh - 1;
      int newLowL = juce::jmin(limitLow, newLowH);
      int newLow  = juce::jlimit(newLowL, newLowH, initialVelLow + velocityDelta);
      if (currentMapping.velocityLow != newLow) {
        currentMapping.velocityLow = newLow;
        boundsChanged = true;
      }
    }
    // -------------------------------------------------------
    // BODY DRAG
    // -------------------------------------------------------
    else if (currentDragMode == DragMode::Body) {
      if (isAltDrag) {
        // ── ALT MODE: fire a swap request when centre crosses into another slot
        // We do NOT change velocityLow/High here (non-destructive).
        int currentCentre = juce::jlimit(0, 127,
            initialCenterVel + velocityDelta);

        // Compute deltaVelCenter relative to original centre
        int deltaVelCenter = currentCentre - initialCenterVel;

        // Only fire once per threshold crossing (guard against rapid re-fire)
        if (!swapDispatched && std::abs(deltaVelCenter) > (initialVelHigh - initialVelLow) / 2 + 2) {
          swapDispatched = true;
          if (onVerticalSwapRequest)
            onVerticalSwapRequest(deltaVelCenter, e.getScreenY());
          // Component may be repositioned by performSwap but NOT destroyed.
          // safeThis confirms we survived the call.
        }
        // No boundsChanged, no onBoundsChanged — purely informational drag
      } else {
        // ── NORMAL MODE: body is LOCKED vertically, no vertical movement.
        // Only horizontal movement (onRequestMove) is active below.
        // We intentionally do NOT update velocityLow/High.
        boundsChanged = false;
      }
    }

    // -------------------------------------------------------
    // HORIZONTAL DRAG (move across note columns)
    // Uses absolute screen X to enable continuous multi-column sliding.
    // Passes isAltDrag so MainComponent can skip collision.
    // -------------------------------------------------------
    if (currentDragMode == DragMode::Body && onRequestMove && isAltDrag) {
      int colWidth = juce::jmax(1, getWidth());
      int pixelsMoved = e.getScreenX() - initialScreenX;
      int newNoteOffset = pixelsMoved / colWidth; // rounds toward zero

      int delta = newNoteOffset - lastNoteOffset;
      if (delta != 0) {
        lastNoteOffset = newNoteOffset;
        onRequestMove(delta, isAltDrag);
        if (safeThis == nullptr)
          return;
        // NOTE: initialScreenX is NOT reset here — absolute tracking
        // means the offset accumulates naturally across the drag.
      }
    }

    // -------------------------------------------------------
    // Apply validated bounds and notify parent (handle drags)
    // -------------------------------------------------------
    if (boundsChanged) {
      if (currentDragMode == DragMode::TopHandle) {
        int newHighL = currentMapping.velocityLow + 1;
        int newHighH = juce::jmax(newHighL, limitHigh);
        currentMapping.velocityHigh =
            juce::jlimit(newHighL, newHighH, currentMapping.velocityHigh);
      } else if (currentDragMode == DragMode::BottomHandle) {
        int newLowH = currentMapping.velocityHigh - 1;
        int newLowL = juce::jmin(limitLow, newLowH);
        currentMapping.velocityLow =
            juce::jlimit(newLowL, newLowH, currentMapping.velocityLow);
      }

      if (onBoundsChanged)
        onBoundsChanged(currentMapping);
      if (safeThis == nullptr)
        return;

      repaint();
    }
  }
}

void SampleRegion::mouseUp(const juce::MouseEvent &e) {
  juce::Component::SafePointer<SampleRegion> safeThis(this);
  auto mode = currentDragMode;
  currentDragMode = DragMode::None; // Set to None before callbacks

  if (onAuditionEnd) {
    onAuditionEnd(currentMapping);
  }
  if (safeThis == nullptr)
    return;

  if (mode == DragMode::Eraser) {
    // Final clear at mouseUp location
    if (auto *parent = getParentComponent()) {
      auto screenPos = e.getScreenPosition();
      for (auto *sibling : parent->getChildren()) {
        if (auto *other = dynamic_cast<SampleRegion *>(sibling)) {
          if (other->getScreenBounds().contains(screenPos.toInt())) {
            other->currentMapping.samplePath = "";
            if (other->onErase)
              other->onErase();
          }
        }
      }
    }
    // Clear self if dragging started on self
    currentMapping.samplePath = "";
    if (onErase)
      onErase();

    // Trigger ONE final UI update and synth rebuild
    if (onDragFinished)
      onDragFinished(currentMapping);

  } else if (e.mouseWasDraggedSinceMouseDown() && mode != DragMode::None) {
    if (onDragFinished)
      onDragFinished(currentMapping);
    if (safeThis == nullptr)
      return;
  }

  setMouseCursor(juce::MouseCursor::NormalCursor);
  repaint();
}

void SampleRegion::mouseDoubleClick(const juce::MouseEvent &e) {
  juce::Component::SafePointer<SampleRegion> safeThis(this);

  if (auto *parent = getParentComponent()) {
    int limitLow = 0;
    int limitHigh = 127;

    for (auto *sibling : parent->getChildren()) {
      if (auto *otherRegion = dynamic_cast<SampleRegion *>(sibling)) {
        if (otherRegion != this &&
            otherRegion->currentMapping.samplePath.isNotEmpty()) {
          const auto &otherMap = otherRegion->currentMapping;

          // If sibling is below us
          if (otherMap.velocityHigh < currentMapping.velocityLow) {
            limitLow = juce::jmax(limitLow, otherMap.velocityHigh + 1);
          }
          // If sibling is above us
          if (otherMap.velocityLow > currentMapping.velocityHigh) {
            limitHigh = juce::jmin(limitHigh, otherMap.velocityLow - 1);
          }
        }
      }
    }

    if (currentMapping.velocityLow != limitLow ||
        currentMapping.velocityHigh != limitHigh) {
      currentMapping.velocityLow = limitLow;
      currentMapping.velocityHigh = limitHigh;

      if (onBoundsChanged)
        onBoundsChanged(currentMapping);
      if (safeThis == nullptr)
        return;

      if (onDragFinished)
        onDragFinished(currentMapping);
      if (safeThis == nullptr)
        return;

      parent->resized();
      repaint();
    }
  }
}

// Called by MainComponent::performSwap() after each swap completes.
// Resets the swap guard and anchors the drag reference to the new
// velocity position, enabling continuous multi-swap in one mouse gesture.
void SampleRegion::resetSwapState(int newMouseScreenY) {
  swapDispatched = false;
  initialVelLow    = currentMapping.velocityLow;
  initialVelHigh   = currentMapping.velocityHigh;
  initialCenterVel = (initialVelLow + initialVelHigh) / 2;
  dragStartY       = newMouseScreenY;
}

} // namespace sotero
