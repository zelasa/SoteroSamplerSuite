#pragma once

#include "SoteroFormat.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>

namespace sotero {

/**
 * @class SoteroLoopEngine
 * @brief Polyphonic MIDI loop playback engine.
 * Handles up to 36 loops (Abas A, B, C) with BPM sync.
 */
class SoteroLoopEngine {
public:
  SoteroLoopEngine() = default;

  void setLoops(const juce::Array<LoopMapping> &newLoops) {
    loops = newLoops;
    // In a real implementation, we would pre-load/parse MIDI files here.
  }

  void triggerLoop(int slotIndex, bool shouldPlay) {
    if (slotIndex >= 0 && slotIndex < 36) {
      playbackStates[slotIndex] = shouldPlay;

      // Handle Global Cancellation Mode if enabled
      if (shouldPlay && cancellationModeActive) {
        for (int i = 0; i < 36; ++i) {
          if (i != slotIndex)
            playbackStates[i] = false;
        }
      }
    }
  }

  void setCancellationMode(bool active) { cancellationModeActive = active; }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages, double bpm) {
    // Core Logic:
    // 1. Check which loops are active.
    // 2. Map slotIndex to MIDI notes if "MIDI Drag and Drop" trigger is used.
    // 3. Generate MIDI events or sample-accurate triggers.

    currentBpm = bpm;

    // This is a foundation placeholder.
    // Real implementation will involve juce::MidiMessageSequence and sync
    // logic.
  }

private:
  juce::Array<LoopMapping> loops;
  bool playbackStates[36] = {false};
  bool cancellationModeActive = false;
  double currentBpm = 120.0;
};

} // namespace sotero
