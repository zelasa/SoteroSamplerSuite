#pragma once

#include "../../Common/SoteroEngineInterface.h"
#include "../../Common/SoteroFormat.h"
#include "../../Common/SoteroViews.h"
#include "BinaryData.h"
#include "LibraryController.h"
#include "../../Common/UI/ADSRWidget.h"
#include "../../Common/UI/FilterWidget.h"
#include "../../Common/UI/DynamicsWidget.h"
#include "../../Common/UI/HeaderWidget.h"
#include "../../Common/UI/MetadataWidget.h"
#include "../../Common/UI/SculptingWidget.h"
#include "../../Common/UI/AdvancedWidget.h"
#include "../../Common/UI/WaveformWidget.h"
#include "MappingWidget.h"
#include "../../SamplerPlayer/Source/SoteroSamplerVoice.h"
#include "SampleRegion.h"
#include <JuceHeader.h>
#include <memory>

namespace sotero {
/**
 * @class MainComponent
 * @brief The main workspace for SoteroBuilder.
 */
class MainComponent : public juce::AudioAppComponent,
                      public juce::FileDragAndDropTarget,
                      public juce::MidiInputCallback,
                      public ISoteroAudioEngine,
                      public juce::Timer {
public:
  MainComponent();
  ~MainComponent() override;

  void timerCallback() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  // --- AudioAppComponent Overrides ---
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;
  void releaseResources() override;

  // --- MidiInputCallback Overrides ---
  void handleIncomingMidiMessage(juce::MidiInput *source,
                                 const juce::MidiMessage &message) override;

  // --- FileDragAndDropTarget Overrides ---
  bool isInterestedInFileDrag(const juce::StringArray &files) override {
    return true;
  }
  void filesDropped(const juce::StringArray &files, int x, int y) override;

  // --- ISoteroAudioEngine Implementation ---
  void loadSoteroLibrary(const juce::File &file) override;


private:
  // --- Audio ---
  // Internal AudioProcessor dummy for APVTS - MUST BE DECLARED FIRST
  struct DummyProcessor : public juce::AudioProcessor {
    DummyProcessor()
        : AudioProcessor(BusesProperties().withOutput(
              "Output", juce::AudioChannelSet::stereo(), true)) {}
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {
    }
    juce::AudioProcessorEditor *createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "Dummy"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return ""; }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override {}
    void setStateInformation(const void *, int) override {}
  } dummyProcessor;

  // Parameters & Effects (Player Mode) - DECLARED AFTER dummyProcessor
  std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
  juce::dsp::StateVariableTPTFilter<float> masterToneFilter;

  std::unique_ptr<juce::FileChooser> chooser;
  // --- UI Modular Panels ---

  HeaderWidget headerPanel;
  SculptingWidget sculptingPanel;
  AdvancedWidget advancedPanel;
  MetadataWidget metadataPanel;
  MappingWidget mappingPanel;

  std::unique_ptr<WaveformWidget> waveform1, waveform2;

  // --- Data ---
  LibraryController libraryController;
  
  LibraryMetadata libraryData; // We'll keep this for now to avoid breaking everything at once, 
                               // but we'll sync it with libraryController soon.
  juce::Image currentArtwork;
  int activeMappingIndex = -1;
  juce::File lastBrowseDirectory;
  void updateOctave(int newOctave);

  juce::File currentLibraryFile;


  void updateGridUI();
  void updateMetadataFromUI();
  void rebuildSynth();
  void auditionSample(const juce::String &path, int midiNote, int velocity);
  void auditionSampleOff(int midiNote);
  void deselectAllRegions();

  // --- ISoteroAudioEngine Implementation ---
  juce::AudioProcessorValueTreeState &getAPVTS() override { return *apvts; }
  juce::MidiKeyboardState &getKeyboardState() override { return keyboardState; }

  float getLevelL() const override { return lastLevelL.load(); }
  float getLevelR() const override { return lastLevelR.load(); }

  juce::String getLibraryName() const override { return libraryData.name; }
  juce::String getLibraryAuthor() const override { return libraryData.author; }
  juce::String getLibraryDescription() const override {
    return libraryData.description;
  }
  juce::Image getLibraryArtwork() const override { return currentArtwork; }
  bool isLibraryLoaded() const override { return true; }

  int getLastMidiNote() const override { return lastMidiNote.load(); }
  int getLastMidiVelocity() const override { return lastMidiVelocity.load(); }

  // --- UI ---
  std::unique_ptr<juce::Component> editView;

  // --- Audio ---
  class SoteroSynthesiser : public juce::Synthesiser {
  public:
    void noteOn(const int midiChannel, const int midiNoteNumber,
                const float velocity) override {
      const juce::ScopedLock sl(lock);
      for (int i = 0; i < voices.size(); ++i) {
        if (auto *v = dynamic_cast<SoteroSamplerVoice *>(voices.getUnchecked(i))) {
          for (int j = 0; j < sounds.size(); ++j) {
            if (auto *s = dynamic_cast<SoteroSamplerSound *>(
                    sounds.getUnchecked(j).get())) {
              if (s->appliesToNote(midiNoteNumber) &&
                  s->appliesToVelocity((int)(velocity * 127.0f))) {
                v->choke(s->chokeGroupId);
              }
            }
          }
        }
      }
      juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
    }
  };

  juce::AudioFormatManager formatManager;
  SoteroSynthesiser synth;
  juce::CriticalSection synthLock;
  juce::MidiBuffer emptyMidi;
  juce::MidiMessageCollector midiCollector;
  juce::MidiKeyboardState keyboardState;

  // Parameters & Effects (Player Mode)
  juce::dsp::Compressor<float> masterCompressor;
  juce::dsp::Reverb masterReverb;
  juce::dsp::Reverb::Parameters reverbParams;

  // Attachments
  using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
  using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

  std::unique_ptr<ComboAtt> compModeAtt;
  std::unique_ptr<SliderAtt> compThreshAtt, compRatioAtt, compAttackAtt,
      compReleaseAtt;
  std::unique_ptr<ButtonAtt> revEnableAtt;
  std::unique_ptr<SliderAtt> revSizeAtt, revMixAtt, toneAtt, volAtt, pitchAtt;

  std::atomic<float> lastLevelL{0.0f}, lastLevelR{0.0f};
  std::atomic<int> lastMidiNote{-1}, lastMidiVelocity{-1};

  enum class UIMode { Developer, UserPlayer };
  UIMode currentUIMode = UIMode::Developer;
  void setUIMode(UIMode mode);

  void alignLayers();
  bool isRangeFree(int note, int micLayer, int lo, int hi, int excludeIndex);
  void resolveCollisions(int note, int micLayer, int &targetLo, int &targetHi,
                         int excludeIndex, bool allowCrossSync, 
                         bool isPrimaryTarget = true);
  void applyDefinitiveCollision(int targetIndex, const KeyMapping &proposed, int modeVal, bool isSwapPhase);

  void updateColumnRegions(int note, int layer);
  int findCounterpart(int sourceIndex);
  void performSwap(int mIndexA, int mIndexB, SampleRegion* draggedRegion, int mouseScreenY);
  SampleRegion* findRegionForIndex(int mappingIndex); // UI lookup: mapping index -> SampleRegion*
  int dragCounterpartIndex = -1; // Persistent grip for horizontal sync

  bool dragStickyTop = false;
  bool dragStickyBottom = false;

  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
} // namespace sotero
