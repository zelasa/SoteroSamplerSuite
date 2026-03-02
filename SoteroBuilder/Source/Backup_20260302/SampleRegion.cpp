#include "SampleRegion.h"

namespace sotero {

SampleRegion::SampleRegion(const KeyMapping &mapping, int nIdx)
    : currentMapping(mapping), parentNoteIndex(nIdx) {
  setRepaintsOnMouseActivity(true);
}

void SampleRegion::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Base color
  juce::Colour baseColor = juce::Colours::cyan;
  g.setColour(baseColor.withAlpha(isHovering ? 0.8f : 0.6f));
  g.fillRoundedRectangle(bounds.reduced(1.0f), 3.0f);

  // Border
  g.setColour(juce::Colours::white.withAlpha(isHovering ? 0.9f : 0.4f));
  g.drawRoundedRectangle(bounds.reduced(1.0f), 3.0f, isHovering ? 2.0f : 1.0f);

  // Draw handles if hovering
  if (isHovering) {
    g.setColour(juce::Colours::white);
    g.fillRect(getTopHandleBounds().reduced(1));
    g.fillRect(getBottomHandleBounds().reduced(1));
  }

  // Draw Sample Name
  if (currentMapping.fileName.isNotEmpty() &&
      currentDragMode == DragMode::None) {
    g.setColour(juce::Colours::white);
    g.setFont(10.0f);

    // Calculate a safe area for text (away from handles)
    auto textArea = getLocalBounds().reduced(2, 6);
    if (textArea.getHeight() > 10) {
      g.drawFittedText(currentMapping.fileName, textArea,
                       juce::Justification::centred, 2);
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
  int handleH = juce::jmin(8, getLocalBounds().getHeight() / 3);
  return getLocalBounds().removeFromTop(handleH);
}

juce::Rectangle<int> SampleRegion::getBottomHandleBounds() const {
  int handleH = juce::jmin(8, getLocalBounds().getHeight() / 3);
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
  toFront(true); // Bring to front when clicked so it's not hidden by
                 // overlapping regions

  if (e.mods.isRightButtonDown() || e.mods.isAltDown()) {
    if (onClear)
      onClear(currentMapping);
    return;
  }

  if (getTopHandleBounds().contains(e.getPosition())) {
    currentDragMode = DragMode::TopHandle;
  } else if (getBottomHandleBounds().contains(e.getPosition())) {
    currentDragMode = DragMode::BottomHandle;
  } else {
    currentDragMode = DragMode::Body;
  }

  dragStartY = e.getScreenY();
  initialVelLow = currentMapping.velocityLow;
  initialVelHigh = currentMapping.velocityHigh;
}

void SampleRegion::mouseDrag(const juce::MouseEvent &e) {
  if (currentDragMode == DragMode::None || e.mods.isRightButtonDown())
    return;

  // We need to know the height of the parent column to calculate velocity
  // accurately. Parent height represents 128 velocity steps (0 to 127).
  if (auto *parent = getParentComponent()) {
    float parentHeight = (float)parent->getHeight();
    if (parentHeight <= 0)
      return;

    float pixelsPerVelocity = parentHeight / 128.0f;
    int dragDeltaY = e.getScreenY() - dragStartY;

    // Negative Y is up (higher velocity), Positive Y is down (lower velocity)
    int velocityDelta = juce::roundToInt(-dragDeltaY / pixelsPerVelocity);

    bool boundsChanged = false;

    if (currentDragMode == DragMode::TopHandle) {
      int newHigh = juce::jlimit(currentMapping.velocityLow + 1, 127,
                                 initialVelHigh + velocityDelta);
      if (currentMapping.velocityHigh != newHigh) {
        currentMapping.velocityHigh = newHigh;
        boundsChanged = true;
      }
    } else if (currentDragMode == DragMode::BottomHandle) {
      int newLow = juce::jlimit(0, currentMapping.velocityHigh - 1,
                                initialVelLow + velocityDelta);
      if (currentMapping.velocityLow != newLow) {
        currentMapping.velocityLow = newLow;
        boundsChanged = true;
      }
    } else if (currentDragMode == DragMode::Body) {
      int range = initialVelHigh - initialVelLow;
      if (range < 1)
        range = 1;

      int newLow = juce::jlimit(0, 127 - range, initialVelLow + velocityDelta);
      int newHigh = newLow + range;

      if (currentMapping.velocityLow != newLow ||
          currentMapping.velocityHigh != newHigh) {
        currentMapping.velocityLow = newLow;
        currentMapping.velocityHigh = newHigh;
        boundsChanged = true;
      }
    }

    if (boundsChanged) {
      // Apply global limits only - allowing "pass through" behavior during drag
      if (currentDragMode == DragMode::TopHandle) {
        currentMapping.velocityHigh = juce::jlimit(
            currentMapping.velocityLow + 1, 127, currentMapping.velocityHigh);
      } else if (currentDragMode == DragMode::BottomHandle) {
        currentMapping.velocityLow = juce::jlimit(
            0, currentMapping.velocityHigh - 1, currentMapping.velocityLow);
      } else if (currentDragMode == DragMode::Body) {
        int range = initialVelHigh - initialVelLow;
        currentMapping.velocityLow =
            juce::jlimit(0, 127 - range, currentMapping.velocityLow);
        currentMapping.velocityHigh = currentMapping.velocityLow + range;
      }

      if (onBoundsChanged)
        onBoundsChanged(currentMapping);
      repaint();
    }
  }
}

void SampleRegion::mouseUp(const juce::MouseEvent &e) {
  if (currentDragMode == DragMode::Body && !e.mouseWasDraggedSinceMouseDown()) {
    // It was a click!
    if (onAudition && (e.mods.isCtrlDown() || e.mods.isCommandDown()))
      onAudition(currentMapping);
  } else if (e.mouseWasDraggedSinceMouseDown() &&
             currentDragMode != DragMode::None) {

    // Reconciliation: If we dropped on another sample, snap to the nearest edge
    // Reconciliation: Drop the sample into the nearest available gap
    if (auto *parent = getParentComponent()) {
      int targetLow = currentMapping.velocityLow;
      int targetHigh = currentMapping.velocityHigh;
      int range = targetHigh - targetLow;

      // 1. Collect all other occupied ranges in this column
      struct R {
        int low, high;
      };
      juce::Array<R> occupied;
      for (auto *sibling : parent->getChildren()) {
        if (auto *other = dynamic_cast<SampleRegion *>(sibling)) {
          if (other != this && other->currentMapping.samplePath.isNotEmpty()) {
            occupied.add({other->currentMapping.velocityLow,
                          other->currentMapping.velocityHigh});
          }
        }
      }

      // Sort occupied ranges by velocity
      std::sort(occupied.begin(), occupied.end(),
                [](const R &a, const R &b) { return a.low < b.low; });

      // 2. Check for overlaps
      bool overlapping = false;
      for (const auto &r : occupied) {
        if (targetLow <= r.high && targetHigh >= r.low) {
          overlapping = true;
          break;
        }
      }

      if (overlapping) {
        // 3. Find all available gaps
        juce::Array<R> gaps;
        int lastHigh = -1;
        for (const auto &r : occupied) {
          if (r.low > lastHigh + 1) {
            gaps.add({lastHigh + 1, r.low - 1});
          }
          lastHigh = r.high;
        }
        if (lastHigh < 127) {
          gaps.add({lastHigh + 1, 127});
        }

        // 4. Find the gap closest to where we dropped
        float targetCenter = (targetLow + targetHigh) / 2.0f;
        int bestGapIdx = -1;
        float minDistance = 1000.0f;

        for (int i = 0; i < gaps.size(); ++i) {
          float gapCenter = (gaps[i].low + gaps[i].high) / 2.0f;
          float dist = std::abs(gapCenter - targetCenter);
          if (dist < minDistance) {
            minDistance = dist;
            bestGapIdx = i;
          }
        }

        if (bestGapIdx != -1) {
          const auto &gap = gaps[bestGapIdx];
          int gapSize = gap.high - gap.low; // inclusive size minus 1

          if (gapSize >= range) {
            // Fits! Place it as close to target as possible inside the gap
            targetLow = juce::jlimit(gap.low, gap.high - range, targetLow);
            targetHigh = targetLow + range;
          } else {
            // Doesn't fit? Squash it to fill the gap
            targetLow = gap.low;
            targetHigh = gap.high;
          }
        }
      }

      currentMapping.velocityLow = targetLow;
      currentMapping.velocityHigh = targetHigh;

      if (onBoundsChanged)
        onBoundsChanged(currentMapping);
    }

    if (onDragFinished)
      onDragFinished(currentMapping);
  }

  currentDragMode = DragMode::None;
  setMouseCursor(juce::MouseCursor::NormalCursor);
  repaint();
}

void SampleRegion::mouseDoubleClick(const juce::MouseEvent &e) {
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

      if (onDragFinished)
        onDragFinished(currentMapping);

      parent->resized();
      repaint();
    }
  }
}

} // namespace sotero
