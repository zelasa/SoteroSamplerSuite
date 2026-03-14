#pragma once

#include "../../Common/SoteroArchive.h"
#include "../../Common/SoteroEngineInterface.h"
#include "../../Common/SoteroEngine.h"
#include "../../Common/SoteroLoopEngine.h"
#include "SoteroSamplerVoice.h"
#include <JuceHeader.h>

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

  // --- ISoteroAudioEngine --- all delegate to engine
  juce::AudioProcessorValueTreeState &getAPVTS() override { return engine->getAPVTS(); }
  juce::MidiKeyboardState &getKeyboardState() override { return engine->getKeyboardState(); }
  float getLevelL() const override { return engine->getLevelL(); }
  float getLevelR() const override { return engine->getLevelR(); }
  juce::String getLibraryName() const override { return engine->getLibraryName(); }
  juce::String getLibraryAuthor() const override { return engine->getLibraryAuthor(); }
  juce::String getLibraryDescription() const override { return engine->getLibraryDescription(); }
  juce::Image getLibraryArtwork() const override { return engine->getLibraryArtwork(); }
  bool isLibraryLoaded() const override { return engine->isLibraryLoaded(); }
  int getLastMidiNote() const override { return engine->getLastMidiNote(); }
  int getLastMidiVelocity() const override { return engine->getLastMidiVelocity(); }

  void loadSoteroLibrary(const juce::File &file) override;
  void auditionMappingStart(int mappingIndex, float velocity) override;
  void auditionMappingStop(int mappingIndex) override;

  juce::File getCurrentLibraryFile() const { return currentLibraryFile; }

  // View management
  bool isPerformanceView() const { return currentView == 0; }
  void setView(int viewIndex) { currentView = viewIndex; }

private:
  std::unique_ptr<sotero::SoteroEngine> engine;
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  juce::File currentLibraryFile;
  std::atomic<int> currentView{0};

  sotero::SoteroLoopEngine loopEngine;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerPlayerAudioProcessor)
};
