#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <cstdint>

namespace sotero {

/**
 * @class CurvedADSR
 * @brief A professional ADSR envelope generator with adjustable curvatures
 * (slopes).
 */
class CurvedADSR {
public:
  struct Parameters {
    float attack = 0.01f; // seconds
    float decay = 0.1f;   // seconds
    float sustain = 1.0f; // level (0 to 1)
    float release = 0.1f; // seconds

    float attackCurve = 0.0f;  // -1.0 (Log/Fast) to 1.0 (Exp/Slow)
    float decayCurve = 0.0f;   // -1.0 to 1.0
    float releaseCurve = 0.0f; // -1.0 to 1.0
  };

  CurvedADSR() {}

  void setSampleRate(double newSampleRate) { sampleRate = newSampleRate; }

  void setParameters(const Parameters &newParams) {
    params = newParams;
    updateRates();
  }

  void noteOn() {
    state = State::Attack;
    currentLevel = 0.0f;
    targetLevel = 1.0f;
    sampleCounter = 0;
    updateRates();
  }

  void noteOff() {
    state = State::Release;
    releaseStartLevel = currentLevel;
    targetLevel = 0.0f;
    sampleCounter = 0;
    updateRates();
  }

  void quickRelease() {
    state = State::Shutdown;
    releaseStartLevel = currentLevel;
    targetLevel = 0.0f;
    sampleCounter = 0;
    shutdownSamples = (juce::int64)(0.01f * sampleRate); // 10ms shutdown
  }

  void reset() {
    state = State::Idle;
    currentLevel = 0.0f;
    sampleCounter = 0;
  }

  bool isActive() const { return state != State::Idle; }

  float getNextSample() {
    if (state == State::Idle)
      return 0.0f;

    if (state == State::Sustain)
      return params.sustain;

    float progress = 0.0f;
    float startL = 0.0f;
    float endL = 1.0f;
    float curve = 0.0f;
    juce::int64 totalS = 0;

    switch (state) {
    case State::Attack:
      totalS = attackSamples;
      startL = 0.0f;
      endL = 1.0f;
      curve = params.attackCurve;
      break;
    case State::Decay:
      totalS = decaySamples;
      startL = 1.0f;
      endL = params.sustain;
      curve = params.decayCurve;
      break;
    case State::Release:
      totalS = releaseSamples;
      startL = releaseStartLevel;
      endL = 0.0f;
      curve = params.releaseCurve;
      break;
    case State::Shutdown:
      totalS = shutdownSamples;
      startL = releaseStartLevel;
      endL = 0.0f;
      curve = 0.0f; // Linear for shutdown
      break;
    default:
      break;
    }

    if (totalS <= 0) {
      currentLevel = endL;
      advanceState();
    } else {
      progress = (float)sampleCounter / (float)totalS;
      float curvedProgress = applyCurve(progress, curve);
      currentLevel = startL + (endL - startL) * curvedProgress;

      sampleCounter++;
      if (sampleCounter >= totalS) {
        currentLevel = endL;
        advanceState();
      }
    }

    return currentLevel;
  }

  void applyEnvelopeToBuffer(juce::AudioBuffer<float> &buffer, int startSample,
                             int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
      float env = getNextSample();
      for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.getWritePointer(ch)[startSample + i] *= env;
      }
    }
  }

  /**
   * Returns the elapsed time in seconds relative to the visual representation.
   * Visual representation assumes: Attack + Decay + (0.5s Sustain) + Release.
   */
  float getVisualTimeElapsed() const {
    if (state == State::Idle)
      return -1.0f;

    float t = 0.0f;
    const float visualSustain = 0.5f;

    switch (state) {
    case State::Attack:
      t = (float)sampleCounter / (float)sampleRate;
      break;
    case State::Decay:
      t = params.attack + ((float)sampleCounter / (float)sampleRate);
      break;
    case State::Sustain:
      t = params.attack + params.decay + (visualSustain * 0.5f);
      break;
    case State::Release:
      t = params.attack + params.decay + visualSustain +
          ((float)sampleCounter / (float)sampleRate);
      break;
    case State::Shutdown:
      t = params.attack + params.decay + visualSustain +
          ((float)sampleCounter / (float)sampleRate);
      break;
    default:
      break;
    }
    return t;
  }

private:
  enum class State { Idle, Attack, Decay, Sustain, Release, Shutdown };

  void advanceState() {
    sampleCounter = 0;
    switch (state) {
    case State::Attack:
      state = State::Decay;
      break;
    case State::Decay:
      state = State::Sustain;
      break;
    case State::Release:
    case State::Shutdown:
      state = State::Idle;
      break;
    default:
      break;
    }
  }

  void updateRates() {
    attackSamples = (int64_t)(params.attack * sampleRate);
    decaySamples = (int64_t)(params.decay * sampleRate);
    releaseSamples = (int64_t)(params.release * sampleRate);
  }

  /**
   * Applies curvature to a normalized [0, 1] value.
   * curve > 0: Exponential (Slow start, fast end)
   * curve < 0: Logarithmic (Fast start, slow end)
   * curve = 0: Linear
   */
  float applyCurve(float x, float curve) {
    if (std::abs(curve) < 0.001f)
      return x;

    // A professional way to handle curves is using the formula:
    // (exp(k*x) - 1) / (exp(k) - 1)
    // where k is our curvature parameter.
    float k = curve * 5.0f; // Scale curve to a useful range
    return (std::exp(k * x) - 1.0f) / (std::exp(k) - 1.0f);
  }

  State state = State::Idle;
  Parameters params;
  double sampleRate = 44100.0;
  float currentLevel = 0.0f;
  float targetLevel = 0.0f;
  float releaseStartLevel = 0.0f;

  juce::int64 sampleCounter = 0;
  juce::int64 attackSamples = 0;
  juce::int64 decaySamples = 0;
  juce::int64 releaseSamples = 0;
  juce::int64 shutdownSamples = 0;
};

} // namespace sotero
