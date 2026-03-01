#pragma once

#include "../../Common/SoteroArchive.h"
#include "SoteroSamplerVoice.h"
#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

class SamplerTrack {
public:
  SamplerTrack() {
    for (auto i = 0; i < 8; ++i)
      synth.addVoice(new sotero::SoteroSamplerVoice());

    formatManager.registerBasicFormats();
  }

  void prepareToPlay(double sampleRate, int samplesPerBlock) {
    synth.setCurrentPlaybackSampleRate(sampleRate);
  }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) {

    // --- Choke Group Logic ---
    for (const auto metadata : midiMessages) {
      auto msg = metadata.getMessage();
      if (msg.isNoteOn()) {
        // Find which sound would be triggered to get its choke group
        for (int i = 0; i < synth.getNumSounds(); ++i) {
          if (auto *sound = dynamic_cast<sotero::SoteroSamplerSound *>(
                  synth.getSound(i).get())) {
            if (sound->appliesToNote(msg.getNoteNumber()) &&
                sound->appliesToVelocity(msg.getVelocity())) {
              int group = sound->chokeGroupId;
              if (group != 0) {
                for (int v = 0; v < synth.getNumVoices(); ++v) {
                  if (auto *voice = dynamic_cast<sotero::SoteroSamplerVoice *>(
                          synth.getVoice(v))) {
                    voice->choke(group);
                  }
                }
              }
              break;
            }
          }
        }
      }
    }

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    level.store(buffer.getMagnitude(0, buffer.getNumSamples()));
  }

  bool loadSample(const juce::File &file) {
    if (!file.existsAsFile())
      return false;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));
    if (reader != nullptr) {
      juce::BigInteger range;
      range.setRange(0, 128, true);
      synth.clearSounds();
      synth.addSound(new juce::SamplerSound("Sample", *reader, range, 60, 0.005,
                                            0.1, 10.0));
      return true;
    }
    return false;
  }

  /**
   * @brief Loads a specific note from a .spsa archive.
   */
  bool loadArchiveNote(const juce::File &archiveFile, int midiNote) {
    auto metadata = sotero::SoteroArchive::readMetadata(archiveFile);
    if (metadata.name.isEmpty())
      return false;

    synth.clearSounds();
    bool loadedAnything = false;

    for (const auto &mapping : metadata.mappings) {
      if (mapping.midiNote == midiNote && !mapping.samplePath.isEmpty()) {
        auto data = sotero::SoteroArchive::extractResource(archiveFile,
                                                           mapping.samplePath);
        if (data.getSize() > 0) {
          auto stream = std::make_unique<juce::MemoryInputStream>(data, false);
          std::unique_ptr<juce::AudioFormatReader> reader(
              formatManager.createReaderFor(std::move(stream)));

          if (reader != nullptr) {
            juce::BigInteger range;
            range.setBit(midiNote);

            juce::BigInteger velocityRange;
            velocityRange.setRange(
                mapping.velocityLow,
                mapping.velocityHigh - mapping.velocityLow + 1, true);

            // Use the target midiNote as the root note to maintain original
            // pitch
            synth.addSound(new sotero::SoteroSamplerSound(
                "NoteLib", *reader, range, midiNote, 0.005, 0.1, 10.0,
                mapping.chokeGroup, mapping.velocityLow, mapping.velocityHigh));

            loadedAnything = true;
          }
        }
      }
    }
    return loadedAnything;
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

  // Unified Sound Engine
  juce::Synthesiser synth;
  juce::AudioFormatManager formatManager;

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
  bool loadSoteroLibrary(const juce::File &file);
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
