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
                     int vLow = 0, int vHigh = 127, int64_t start = 0,
                     int64_t end = 0, int64_t fIn = 0, int64_t fOut = 0,
                     float vol = 1.0f, float fineTune = 0.0f, int micLayer = 0,
                     float a = 0.01f, float d = 0.1f, float s = 1.0f,
                     float r = 0.1f, int fType = 0, float fCut = 20000.0f,
                     float fRes = 1.0f)
      : juce::SamplerSound(name, source, midiNotes, midiRootNote,
                           attackTimeSecs, releaseTimeSecs,
                           maxSampleLengthSecs),
        chokeGroupId(chokeGroup), velocityLow(vLow), velocityHigh(vHigh),
        sampleStart(start), sampleEnd(end), fadeIn(fIn), fadeOut(fOut),
        volumeMultiplier(vol), fineTuneCents(fineTune), micLayer(micLayer),
        filterType(fType), filterCutoff(fCut), filterResonance(fRes) {

    adsrParams.attack = a;
    adsrParams.decay = d;
    adsrParams.sustain = s;
    adsrParams.release = r;
  }

  bool appliesToVelocity(int velocity) const {
    return velocity >= velocityLow && velocity <= velocityHigh;
  }

  int chokeGroupId = 0;
  int velocityLow = 0;
  int velocityHigh = 127;

  // Non-destructive metadata
  int64_t sampleStart = 0;
  int64_t sampleEnd = 0;
  int64_t fadeIn = 0;
  int64_t fadeOut = 0;
  float volumeMultiplier = 1.0f;
  float fineTuneCents = 0.0f;
  int micLayer = 0;

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

      switch (s->filterType) {
      case 1:
        filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        break;
      case 2:
        filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        break;
      case 3:
        filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        break;
      default:
        filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        break; // Default or Bypassed (via cutoff)
      }

      filter.setCutoffFrequency(s->filterType > 0 ? s->filterCutoff : 20000.0f);
      filter.setResonance(s->filterResonance);

    } else {
      currentChokeGroupId = 0;
    }

    sourceSamplePosition = 0;
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

    auto *s = dynamic_cast<const sotero::SoteroSamplerSound *>(
        getCurrentlyPlayingSound().get());
    if (s == nullptr) {
      clearCurrentNote();
      return;
    }

    tempBuffer.setSize(outputBuffer.getNumChannels(), numSamples, false, false,
                       true);
    tempBuffer.clear();

    const double playbackSpeedRatio =
        getLevelMultiplier() * (s->getSampleRate() / getSampleRate());

    // Simple implementation of sample offset and playback
    // In a real implementation, we'd use an interpolator for pitch/fine tune.
    // For this foundation, we'll implement the basic offset logic.

    auto *reader = s->getAudioFormatReader();
    if (reader != nullptr) {
      // 1. Calculate Fine Tune Speed Ratio
      // speedShift = 2 ^ (cents / 1200)
      const double fineTuneRatio = std::pow(2.0, s->fineTuneCents / 1200.0);
      const double speedRatio = playbackSpeedRatio * fineTuneRatio;

      // 2. Read Samples with Offset
      int64_t startInSource = s->sampleStart + (int64_t)sourceSamplePosition;
      int64_t numToRead = (int64_t)numSamples;

      // Safety: Don't exceed sampleEnd
      if (s->sampleEnd > 0 && startInSource + numToRead > s->sampleEnd) {
        numToRead = s->sampleEnd - startInSource;
      }

      if (numToRead > 0) {
        reader->read(&tempBuffer, 0, (int)numToRead, startInSource, true, true);

        // 3. Apply Fades (Non-Destructive)
        for (int i = 0; i < (int)numToRead; ++i) {
          float gain = 1.0f;
          int64_t currentRelative = (int64_t)sourceSamplePosition + i;

          // Fade In
          if (s->fadeIn > 0 && currentRelative < s->fadeIn) {
            gain *= (float)currentRelative / (float)s->fadeIn;
          }

          // Fade Out
          int64_t totalLength = (s->sampleEnd > 0)
                                    ? (s->sampleEnd - s->sampleStart)
                                    : reader->lengthInSamples;
          int64_t distFromEnd = totalLength - currentRelative;
          if (s->fadeOut > 0 && distFromEnd < s->fadeOut) {
            gain *= (float)distFromEnd / (float)s->fadeOut;
          }

          for (int ch = 0; ch < tempBuffer.getNumChannels(); ++ch) {
            tempBuffer.setSample(ch, i, tempBuffer.getSample(ch, i) * gain);
          }
        }

        sourceSamplePosition +=
            (double)numToRead *
            speedRatio; // Move head based on pitch/fine tune
      } else {
        adsr.noteOff();
      }
    }

    // Apply Volume Multiplier from Region
    tempBuffer.applyGain(s->volumeMultiplier);

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

    if (!adsr.isActive() ||
        (s->sampleEnd > 0 &&
         s->sampleStart + sourceSamplePosition >= s->sampleEnd)) {
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
