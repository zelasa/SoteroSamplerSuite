#pragma once

#include <JuceHeader.h>

namespace sotero {
/**
 * @class SoteroSamplerSound
 * @brief A custom SamplerSound that carries Choke Group information.
 */
class SoteroSamplerSound : public juce::SamplerSound {
public:
  SoteroSamplerSound(const juce::String &name, juce::AudioFormatReader &source,
                     const juce::BigInteger &midiNotes, int midiRootNote,
                     double attackTimeSecs, double releaseTimeSecs,
                     double maxSampleLengthSecs, int chokeGroup = 0,
                     int vLow = 0, int vHigh = 127)
      : juce::SamplerSound(name, source, midiNotes, midiRootNote,
                           attackTimeSecs, releaseTimeSecs,
                           maxSampleLengthSecs),
        chokeGroupId(chokeGroup), velocityLow(vLow), velocityHigh(vHigh) {}

  bool appliesToVelocity(int velocity) const {
    return velocity >= velocityLow && velocity <= velocityHigh;
  }

  int chokeGroupId = 0;
  int velocityLow = 0;
  int velocityHigh = 127;
};

/**
 * @class SoteroSamplerVoice
 * @brief A custom SamplerVoice that supports Choke Groups.
 */
class SoteroSamplerVoice : public juce::SamplerVoice {
public:
  SoteroSamplerVoice() = default;

  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override {
    if (auto *s = dynamic_cast<const sotero::SoteroSamplerSound *>(sound)) {
      currentChokeGroupId = s->chokeGroupId;
    } else {
      currentChokeGroupId = 0;
    }

    juce::SamplerVoice::startNote(midiNoteNumber, velocity, sound,
                                  currentPitchWheelPosition);
  }

  /**
   * @brief Check if this voice should be choked by a new note.
   */
  void choke(int chokeGroupId) {
    if (isVoiceActive() && currentChokeGroupId == chokeGroupId &&
        chokeGroupId != 0) {
      clearCurrentNote(); // Instant stop
    }
  }

  int currentChokeGroupId = 0;
};
} // namespace sotero
