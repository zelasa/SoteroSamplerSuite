#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <atomic>
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
    float peakLevel = 1.0f;
    float visualSustain = 0.5f; // seconds
  };

  CurvedADSR() : state(State::Idle), sampleCounter(0) {}

  void setSampleRate(double newSampleRate) { sampleRate = newSampleRate; }

  void setParameters(const Parameters &newParams) {
    params = newParams;
    updateRates();
  }

  void noteOn() {
    state.store(State::Attack);
    currentLevel = 0.0f;
    targetLevel = 1.0f;
    sampleCounter.store(0);
    updateRates();
  }

  void noteOff() {
    state.store(State::Release);
    releaseStartLevel = currentLevel;
    targetLevel = 0.0f;
    sampleCounter.store(0);
    updateRates();
  }

  void quickRelease() {
    state.store(State::Shutdown);
    releaseStartLevel = currentLevel;
    targetLevel = 0.0f;
    sampleCounter.store(0);
    shutdownSamples = (juce::int64)(0.01f * sampleRate); // 10ms shutdown
  }

  void reset() {
    state.store(State::Idle);
    currentLevel = 0.0f;
    sampleCounter.store(0);
  }

  bool isActive() const { return state != State::Idle; }

  float getNextSample() {
    auto currentState = state.load();
    if (currentState == State::Idle)
      return 0.0f;

    if (currentState == State::Sustain) {
      sampleCounter.fetch_add(1);
      return params.sustain;
    }

    float progress = 0.0f;
    float startL = 0.0f;
    float endL = 1.0f;
    float curve = 0.0f;
    juce::int64 totalS = 0;

    switch (currentState) {
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
      auto currentSamples = sampleCounter.load();
      progress = (float)currentSamples / (float)totalS;
      float curvedProgress = applyCurve(progress, curve);
      currentLevel = startL + (endL - startL) * curvedProgress;

      sampleCounter.fetch_add(1);
      if (sampleCounter.load() >= totalS) {
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

  void advance(int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
      getNextSample();
    }
  }

  /**
   * Returns the elapsed time in seconds relative to the visual representation.
   * Visual representation assumes: Attack + Decay + (0.5s Sustain) + Release.
   */
  float getVisualTimeElapsed() const {
    auto currentState = state.load();
    auto currentSamples = sampleCounter.load();

    if (currentState == State::Idle)
      return -1.0f;

    switch (currentState) {
    case State::Attack:
      return (float)currentSamples / (float)sampleRate;
    case State::Decay:
      return params.attack + ((float)currentSamples / (float)sampleRate);
    case State::Sustain: {
      // Crawl at real-time speed relative to the visual segment width
      float sustainProgress =
          std::min(1.0f, (float)((double)currentSamples /
                                 (sampleRate * params.visualSustain)));
      return params.attack + params.decay +
             (params.visualSustain * sustainProgress);
    }
    case State::Release:
    case State::Shutdown:
      return params.attack + params.decay + params.visualSustain +
             ((float)currentSamples / (float)sampleRate);
    default:
      return -1.0f;
    }
  }

private:
  enum class State { Idle, Attack, Decay, Sustain, Release, Shutdown };

  void advanceState() {
    sampleCounter.store(0);
    switch (state.load()) {
    case State::Attack:
      state.store(State::Decay);
      break;
    case State::Decay:
      state.store(State::Sustain);
      break;
    case State::Release:
    case State::Shutdown:
      state.store(State::Idle);
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

  std::atomic<State> state;
  Parameters params;
  double sampleRate = 44100.0;
  float currentLevel = 0.0f;
  float targetLevel = 0.0f;
  float releaseStartLevel = 0.0f;

  std::atomic<juce::int64> sampleCounter;
  juce::int64 attackSamples = 0;
  juce::int64 decaySamples = 0;
  juce::int64 releaseSamples = 0;
  juce::int64 shutdownSamples = 0;
};

} // namespace sotero
