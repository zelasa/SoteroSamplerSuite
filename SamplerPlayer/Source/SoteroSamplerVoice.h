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
        chokeGroupId(chokeGroup), velocityLow(vLow), velocityHigh(vHigh) {

    // Initial default ADSR
    adsrParams.attack = (float)attackTimeSecs;
    adsrParams.decay = 0.1f;
    adsrParams.sustain = 1.0f;
    adsrParams.release = (float)releaseTimeSecs;
  }

  bool appliesToVelocity(int velocity) const {
    return velocity >= velocityLow && velocity <= velocityHigh;
  }

  int chokeGroupId = 0;
  int velocityLow = 0;
  int velocityHigh = 127;

  juce::ADSR::Parameters adsrParams;

  // Filter parameters (Foundation)
  float filterCutoff = 20000.0f;
  float filterResonance = 0.1f;
  int filterType = 0; // 0: Bypass, 1: LowPass, 2: HighPass, 3: BandPass
};

/**
 * @class SoteroSamplerVoice
 * @brief A custom SamplerVoice that supports Choke Groups, ADSR, and Filters.
 */
class SoteroSamplerVoice : public juce::SamplerVoice {
public:
  SoteroSamplerVoice() {
    auto &lowPass = processorChain.get<0>();
    lowPass.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
  }

  void prepare(const juce::dsp::ProcessSpec &spec) {
    tempBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
    adsr.setSampleRate(spec.sampleRate);
    processorChain.prepare(spec);
  }

  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override {
    if (auto *s = dynamic_cast<const sotero::SoteroSamplerSound *>(sound)) {
      currentChokeGroupId = s->chokeGroupId;
      adsr.setParameters(s->adsrParams);

      auto &filter = processorChain.get<0>();
      filter.setCutoffFrequency(s->filterCutoff);
      filter.setResonance(s->filterResonance);

    } else {
      currentChokeGroupId = 0;
    }

    juce::SamplerVoice::startNote(midiNoteNumber, velocity, sound,
                                  currentPitchWheelPosition);
    adsr.noteOn();
  }

  void stopNote(float velocity, bool allowTailOff) override {
    if (allowTailOff) {
      adsr.noteOff();
    } else {
      adsr.reset();
      clearCurrentNote();
    }
  }

  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override {
    if (!isVoiceActive())
      return;

    // 1. Render raw sample into temp buffer
    tempBuffer.setSize(outputBuffer.getNumChannels(), numSamples, false, false,
                       true);
    tempBuffer.clear();

    juce::SamplerVoice::renderNextBlock(tempBuffer, 0, numSamples);

    // 2. Apply ADSR
    adsr.applyEnvelopeToBuffer(tempBuffer, 0, numSamples);

    // 3. Apply Filter
    juce::dsp::AudioBlock<float> block(tempBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    processorChain.process(context);

    // 4. Add to output
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      outputBuffer.addFrom(ch, startSample, tempBuffer,
                           ch % tempBuffer.getNumChannels(), 0, numSamples);
    }

    if (!adsr.isActive()) {
      clearCurrentNote();
    }
  }

  /**
   * @brief Check if this voice should be choked by a new note.
   */
  void choke(int chokeGroupId) {
    if (isVoiceActive() && currentChokeGroupId == chokeGroupId &&
        chokeGroupId != 0) {
      adsr.reset(); // Instant stop for choke
      clearCurrentNote();
    }
  }

private:
  int currentChokeGroupId = 0;
  juce::ADSR adsr;
  juce::AudioBuffer<float> tempBuffer;

  juce::dsp::ProcessorChain<juce::dsp::StateVariableTPTFilter<float>>
      processorChain;
};

} // namespace sotero
