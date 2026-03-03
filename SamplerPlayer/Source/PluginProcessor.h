#pragma once

#include "../../Common/SoteroArchive.h"
#include "../../Common/SoteroEngineInterface.h"
#include "SoteroSamplerVoice.h"
#include <JuceHeader.h>

// Unified Sound Engine Components

class SamplerPlayerAudioProcessor : public juce::AudioProcessor,
                                    public sotero::ISoteroAudioEngine {
public:
  SamplerPlayerAudioProcessor();
  ~SamplerPlayerAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState &getAPVTS() override { return apvts; }
  juce::AudioProcessorValueTreeState apvts;

  // Unified Sound Engine
  juce::Synthesiser synth;
  juce::AudioFormatManager formatManager;

  float getLevelL() const override { return lastMasterLevelL.load(); }
  float getLevelR() const override { return lastMasterLevelR.load(); }
  juce::String getLibraryName() const override { return currentLibraryName; }
  juce::String getLibraryAuthor() const override {
    return currentLibraryAuthor;
  }
  int getLastMidiNote() const override { return lastMidiNote.load(); }
  int getLastMidiVelocity() const override { return lastMidiVelocity.load(); }
  juce::MidiKeyboardState &getKeyboardState() override { return keyboardState; }

  // View management
  bool isPerformanceView() const { return currentView == 0; }
  void setView(int viewIndex) { currentView = viewIndex; }

  bool loadTrackSample(int index, const juce::File &file);
  void loadSoteroLibrary(const juce::File &file) override;
  juce::File getCurrentLibraryFile() const { return currentLibraryFile; }

private:
  juce::File currentLibraryFile;
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  juce::String currentLibraryName;
  juce::String currentLibraryAuthor;
  std::atomic<int> lastMidiNote{-1};
  std::atomic<int> lastMidiVelocity{-1};
  std::atomic<int> currentView{0}; // 0: Performance, 1: Setup

  juce::LinearSmoothedValue<float> smoothedGains[3];
  juce::LinearSmoothedValue<float> smoothedPans[3];

  // Effects
  juce::dsp::Compressor<float> masterCompressor;
  juce::dsp::Reverb masterReverb;
  juce::dsp::Reverb::Parameters reverbParams;

  juce::MidiKeyboardState keyboardState;

  std::atomic<float> lastMasterLevelL{0.0f};
  std::atomic<float> lastMasterLevelR{0.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerPlayerAudioProcessor)
};
