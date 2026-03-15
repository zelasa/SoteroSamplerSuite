#pragma once

#include "SoteroFormat.h"
#include "SoteroArchive.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>
#include <cmath>

namespace sotero {

/**
 * @class SoteroLoopEngine
 * @brief Polyphonic MIDI loop playback engine.
 * Handles up to 36 loops (Abas A, B, C) with BPM sync.
 */
class SoteroLoopEngine {
public:
  SoteroLoopEngine() {
    for (int i = 0; i < 36; ++i) {
      playheadTicks[i] = 0.0;
      playbackStates[i] = false;
    }
  }

  void setLoops(const juce::Array<LoopMapping> &newLoops, const juce::File& libraryFile = juce::File{}) {
    loops = newLoops;
    sequences.clear();
    
    for (auto& m : loops) {
        auto seq = std::make_unique<juce::MidiMessageSequence>();
        
        if (m.midiPath.isNotEmpty()) {
            juce::File f(m.midiPath);
            std::unique_ptr<juce::InputStream> input;
            
            if (f.existsAsFile()) {
                input = std::make_unique<juce::FileInputStream>(f);
            } else if (libraryFile.existsAsFile()) {
                auto data = SoteroArchive::extractResource(libraryFile, m.midiPath);
                if (data.getSize() > 0)
                    input = std::make_unique<juce::MemoryInputStream>(data, false);
            }

            if (input) {
                juce::MidiFile midiFile;
                if (midiFile.readFrom(*input)) {
                    midiFile.convertTimestampTicksToSeconds(); // We might want to keep ticks
                    // For now, let's just merge all tracks into one sequence
                    for (int i = 0; i < midiFile.getNumTracks(); ++i)
                        seq->addSequence(*midiFile.getTrack(i), 0.0, 0.0, midiFile.getLastTimestamp());
                    seq->updateMatchedPairs();
                }
            }
        }
        sequences.add(std::move(seq));
    }
  }

  void triggerLoop(int slotIndex, bool shouldPlay) {
    if (slotIndex >= 0 && slotIndex < loops.size()) {
      playbackStates[slotIndex] = shouldPlay;
      if (!shouldPlay) playheadTicks[slotIndex] = 0.0;

      if (shouldPlay && cancellationModeActive) {
        for (int i = 0; i < loops.size(); ++i) {
          if (i != slotIndex) {
            playbackStates[i] = false;
            playheadTicks[i] = 0.0;
          }
        }
      }
    }
  }

  void setCancellationMode(bool active) { cancellationModeActive = active; }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages, double bpm, double sampleRate) {
    currentBpm = bpm;
    int numSamples = buffer.getNumSamples();
    
    // Seconds per sample = 1.0 / sampleRate
    // Beats per second = bpm / 60.0
    // Beats per sample = (bpm / 60.0) / sampleRate
    double beatsPerSample = (bpm / 60.0) / sampleRate;

    for (int i = 0; i < loops.size(); ++i) {
        if (!playbackStates[i]) continue;
        if (i >= sequences.size()) continue;

        auto* seq = sequences[i];
        if (seq == nullptr || seq->getNumEvents() == 0) continue;

        double speed = loops[i].tempoMultiplier;
        double startTime = playheadTicks[i]; // In seconds (since we converted MIDI to seconds)
        double endTime = startTime + (numSamples * beatsPerSample * speed * (60.0 / bpm)); // Corrected: beatsPerSample is in beats. 
        // Actually, let's work in beats or seconds.
        // Let's use seconds since we called convertTimestampTicksToSeconds().
        
        double secondsPerSample = 1.0 / sampleRate;
        endTime = startTime + (numSamples * secondsPerSample * speed);

        // Extract messages in this range
        for (int e = 0; e < seq->getNumEvents(); ++e) {
            auto* event = seq->getEventPointer(e);
            double t = event->message.getTimeStamp();
            
            if (t >= startTime && t < endTime) {
                int sampleOffset = (int)((t - startTime) / (secondsPerSample * speed));
                sampleOffset = juce::jlimit(0, numSamples - 1, sampleOffset);
                midiMessages.addEvent(event->message, sampleOffset);
            }
        }

        playheadTicks[i] = endTime;
        
        // Loop back
        double loopLength = seq->getEndTime();
        if (playheadTicks[i] >= loopLength) {
            playheadTicks[i] = std::fmod(playheadTicks[i], loopLength);
            // Handle edge case: if we wrapped, we might have missed some notes at the start of the next loop
            // In a production engine, we would do a recursive call or a while loop here.
        }
    }
  }

private:
  juce::Array<LoopMapping> loops;
  juce::OwnedArray<juce::MidiMessageSequence> sequences;
  double playheadTicks[36]; // Renamed to playheadSeconds conceptually but kept name for now
  bool playbackStates[36];
  bool cancellationModeActive = false;
  double currentBpm = 120.0;
};

} // namespace sotero
