#pragma once

#include "../../Common/CurvedADSR.h"
#include <JuceHeader.h>
#include <atomic>
#include <cmath>

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
                     int vLow = 0, int vHigh = 127, juce::int64 start = 0,
                     juce::int64 end = 0, juce::int64 fIn = 0,
                     juce::int64 fOut = 0, float vol = 1.0f,
                     float fineTune = 0.0f, int micLayer = 0, float a = 0.01f,
                     float d = 0.1f, float s = 1.0f, float r = 0.1f,
                     float ac = 0.0f, float dc = 0.0f, float rc = 0.0f,
                     int fType = 0, float fCut = 20000.0f, float fRes = 1.0f,
                     bool eAdsr = true, bool eFilter = true,
                     float vSustain = 0.5f)
      : juce::SamplerSound(name, source, midiNotes, midiRootNote,
                           attackTimeSecs, releaseTimeSecs,
                           maxSampleLengthSecs),
        chokeGroupId(chokeGroup), velocityLow(vLow), velocityHigh(vHigh),
        sampleStart(start), sampleEnd(end), fadeIn(fIn), fadeOut(fOut),
        volumeMultiplier(vol), fineTuneCents(fineTune), micLayer(micLayer),
        midiRootNote(midiRootNote), filterType(fType), filterCutoff(fCut),
        filterResonance(fRes) {

    sampleRate = source.sampleRate;
    adsrParams.attack = a;
    adsrParams.decay = d;
    adsrParams.sustain = s;
    adsrParams.release = r;
    adsrParams.attackCurve = ac;
    adsrParams.decayCurve = dc;
    adsrParams.releaseCurve = rc;
    adsrParams.visualSustain = vSustain;
    engineEnableADSR = eAdsr;
    engineEnableFilter = eFilter;
  }

  bool appliesToVelocity(int velocity) const {
    return velocity >= velocityLow && velocity <= velocityHigh;
  }

  int midiRootNote = 0;
  int chokeGroupId = 0;
  int velocityLow = 0;
  int velocityHigh = 127;

  // Non-destructive metadata
  juce::int64 sampleStart = 0;
  juce::int64 sampleEnd = 0;
  juce::int64 fadeIn = 0;
  juce::int64 fadeOut = 0;
  float volumeMultiplier = 1.0f;
  float fineTuneCents = 0.0f;
  int micLayer = 0;

  sotero::CurvedADSR::Parameters adsrParams;

  // Filter parameters (Foundation)
  float filterCutoff = 20000.0f;
  float filterResonance = 0.1f;
  int filterType = 0; // 0: Bypass, 1: LowPass, 2: HighPass, 3: BandPass

  // Master Bypasses
  bool engineEnableADSR = true;
  bool engineEnableFilter = true;

  // Helpers to bypass JUCE 8 private access
  double getSampleRate() const { return sampleRate; }
  int getMidiRootNote() const { return midiRootNote; }
  juce::AudioBuffer<float> *getAudioData() const {
    return juce::SamplerSound::getAudioData();
  }

private:
  double sampleRate = 44100.0;
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
    tempBuffer.setSize(spec.numChannels, (int)spec.maximumBlockSize);
    if (spec.sampleRate > 0.0)
      adsr.setSampleRate(spec.sampleRate);
    processorChain.prepare(spec);
  }

  void reset() {
    processorChain.reset();
    adsr.reset();
  }

  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override {
    currentSamplePosition = 0;
    this->velocity = velocity;

    if (auto *s = dynamic_cast<const sotero::SoteroSamplerSound *>(sound)) {
      currentChokeGroupId = s->chokeGroupId;
      adsr.setParameters(s->adsrParams);

      auto &filter = processorChain.get<0>();
      processorChain.setBypassed<0>(s->filterType == 0 ||
                                    !s->engineEnableFilter);

      if (s->filterType > 0 && s->engineEnableFilter) {
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
          break;
        }

        filter.setCutoffFrequency(s->filterCutoff);
        // Use 1.0 / sqrt(2) as a safe minimum fallback to avoid resonance
        // issues and sounding weird
        filter.setResonance(juce::jmax(
            0.1f, s->filterResonance > 0.0f ? s->filterResonance : 0.707f));
      }

    } else {
      currentChokeGroupId = 0;
    }

    auto sr = getSampleRate();
    if (sr > 0.0)
      adsr.setSampleRate(sr);
    lastTriggerTime.store(juce::Time::getMillisecondCounter());
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

    const double playbackSpeedRatio = s->getSampleRate() / getSampleRate();

    // Simple implementation of sample offset and playback
    // In a real implementation, we'd use an interpolator for pitch/fine tune.
    // For this foundation, we'll implement the basic offset logic.

    auto *audioData = s->getAudioData();
    if (audioData != nullptr) {
      // 1. Calculate Fine Tune Speed Ratio
      const double fineTuneRatio = std::pow(2.0, s->fineTuneCents / 1200.0);
      const double masterPitchRatio =
          std::pow(2.0, (double)masterPitchSemitones / 12.0);
      const double speedRatio =
          playbackSpeedRatio * fineTuneRatio * masterPitchRatio;

      // 2. Read Samples with Offset
      juce::int64 startInSource =
          s->sampleStart + (juce::int64)currentSamplePosition;
      int numToRead = numSamples;

      // Safety: Don't exceed sampleEnd
      juce::int64 endPos =
          s->sampleEnd > 0 ? s->sampleEnd : audioData->getNumSamples();
      if (startInSource + numToRead > endPos) {
        numToRead = (int)(endPos - startInSource);
      }

      if (numToRead > 0) {
        for (int ch = 0; ch < tempBuffer.getNumChannels(); ++ch) {
          tempBuffer.copyFrom(ch, 0, *audioData,
                              ch % audioData->getNumChannels(),
                              (int)startInSource, numToRead);
        }

        // 3. Apply Fades (Non-Destructive)
        for (int i = 0; i < numToRead; ++i) {
          float gain = 1.0f;
          juce::int64 currentRelative = (juce::int64)currentSamplePosition + i;

          // Fade In
          if (s->fadeIn > 0 && currentRelative < s->fadeIn) {
            gain *= (float)currentRelative / (float)s->fadeIn;
          }

          // Fade Out
          juce::int64 totalLength = (s->sampleEnd > 0)
                                        ? (s->sampleEnd - s->sampleStart)
                                        : audioData->getNumSamples();
          juce::int64 distFromEnd = totalLength - currentRelative;
          if (s->fadeOut > 0 && distFromEnd < s->fadeOut) {
            gain *= (float)distFromEnd / (float)s->fadeOut;
          }

          for (int ch = 0; ch < tempBuffer.getNumChannels(); ++ch) {
            tempBuffer.setSample(ch, i, tempBuffer.getSample(ch, i) * gain);
          }
        }

        currentSamplePosition +=
            (double)numToRead *
            speedRatio; // Move head based on pitch/fine tune
      } else {
        adsr.noteOff();
      }
    }

    // Apply Volume Multiplier from Region and Note Velocity
    tempBuffer.applyGain(s->volumeMultiplier * this->velocity);

    // 2. Apply ADSR (Always advance for visualizer tracking)
    if (s->engineEnableADSR) {
      adsr.applyEnvelopeToBuffer(tempBuffer, 0, numSamples);
    } else {
      adsr.advance(numSamples);
    }

    // 3. Apply Filter (Handled automatically by setBypassed<0>)
    juce::dsp::AudioBlock<float> block(tempBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    processorChain.process(context);

    // 4. Add to output
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      outputBuffer.addFrom(ch, startSample, tempBuffer,
                           ch % tempBuffer.getNumChannels(), 0, numSamples);
    }

    // Check if the voice finished its life due to sample end or ADSR completion
    if (s->sampleEnd > 0 &&
        s->sampleStart + currentSamplePosition >= s->sampleEnd) {
      clearCurrentNote();
    } else if (s->engineEnableADSR && !adsr.isActive()) {
      clearCurrentNote();
    }
  }

  /**
   * @brief Check if this voice should be choked by a new note.
   */
  void choke(int chokeGroupId) {
    if (isVoiceActive() && currentChokeGroupId == chokeGroupId &&
        chokeGroupId != 0) {
      adsr.quickRelease(); // Professional shutdown instead of instant reset
    }
  }

  void setMasterPitch(float semitones) { masterPitchSemitones = semitones; }

  float getADSRProgress() const { return adsr.getVisualTimeElapsed(); }

  uint32_t getTriggerTime() const { return lastTriggerTime.load(); }

private:
  int currentChokeGroupId = 0;
  float masterPitchSemitones = 0.0f;
  double currentSamplePosition = 0;
  float velocity = 0;
  std::atomic<uint32_t> lastTriggerTime{0};
  sotero::CurvedADSR adsr;
  juce::AudioBuffer<float> tempBuffer;

  juce::dsp::ProcessorChain<juce::dsp::StateVariableTPTFilter<float>>
      processorChain;
};

} // namespace sotero
