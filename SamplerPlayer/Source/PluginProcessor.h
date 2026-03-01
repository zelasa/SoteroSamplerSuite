#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

class SamplerTrack {
public:
  SamplerTrack() {
    for (auto i = 0; i < 8; ++i)
      synth.addVoice(new juce::SamplerVoice());

    formatManager.registerBasicFormats();
  }

  void prepareToPlay(double sampleRate, int samplesPerBlock) {
    synth.setCurrentPlaybackSampleRate(sampleRate);
  }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) {
    // Aplicar volume e pan serão feitos na mixagem principal do processador
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // VU Meter
    level.store(buffer.getMagnitude(0, buffer.getNumSamples()));
  }

  bool loadSample(const juce::File &file) {
    if (!file.existsAsFile())
      return false;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));
    if (reader != nullptr) {
      juce::BigInteger range;
      range.setRange(0, 128,
                     true); // Respond to all notes, we filter in processBlock

      synth.clearSounds();
      // Use 60 as fixed root note for original pitch
      synth.addSound(
          new juce::SamplerSound("Sample", *reader, range, 60, 0.0, 0.1, 10.0));
      return true;
    }
    return false;
  }

  float getLevel() const { return level.load(); }
  juce::Synthesiser &getSynth() { return synth; }

private:
  juce::Synthesiser synth;
  juce::AudioFormatManager formatManager;
  std::atomic<float> level{0.0f};
};

class SamplerPlayerAudioProcessor : public juce::AudioProcessor {
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

  juce::AudioProcessorValueTreeState apvts;
  SamplerTrack tracks[3];

  float getTrackLevel(int trackIndex) const {
    return tracks[trackIndex].getLevel();
  }
  float getMasterLevelL() const { return lastMasterLevelL.load(); }
  float getMasterLevelR() const { return lastMasterLevelR.load(); }
  juce::String getTrackSampleName(int index) const {
    return trackSampleNames[index];
  }
  int getLastMidiNote() const { return lastMidiNote.load(); }
  int getLastMidiVelocity() const { return lastMidiVelocity.load(); }
  juce::MidiKeyboardState &getKeyboardState() { return keyboardState; }

  // View management
  bool isPerformanceView() const { return currentView == 0; }
  void setView(int viewIndex) { currentView = viewIndex; }

  bool loadTrackSample(int index, const juce::File &file);

private:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  juce::String trackSampleNames[3];
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
