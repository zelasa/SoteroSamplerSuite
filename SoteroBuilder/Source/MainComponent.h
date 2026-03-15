#pragma once

#include <JuceHeader.h>
#include <memory>

#include "../../Common/SoteroEngineInterface.h"
#include "../../Common/SoteroFormat.h"
#include "../../Common/SoteroViews.h"
#include "../../Common/SoteroEngine.h"
#include "../../Common/SoteroMetadata.h"
#include "../../Common/SoteroArchive.h"
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
#include "../../Common/UI/SoteroLookAndFeel.h"
#include "../../SamplerPlayer/Source/SoteroSamplerVoice.h"
#include "SampleRegion.h"

namespace sotero {
/**
 * @class MainComponent
 * @brief The main workspace for SoteroBuilder.
 */
class MainComponent : public juce::AudioAppComponent,
                      public juce::FileDragAndDropTarget,
                      public juce::MidiInputCallback,
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

  // --- Engine Access ---
  sotero::ISoteroAudioEngine& getEngine() { return *engine; }

  void loadSoteroLibrary(const juce::File &file);

private:
  std::unique_ptr<SoteroEngine> engine;

  std::unique_ptr<juce::FileChooser> chooser;
  // --- UI Components ---
  sotero::SoteroLookAndFeel lookAndFeel;
  HeaderWidget headerPanel;
  SculptingWidget sculptingPanel;
  AdvancedWidget advancedPanel;
  MetadataWidget metadataPanel;
  MappingWidget mappingPanel;

  std::unique_ptr<WaveformWidget> waveform1, waveform2;

  // --- Data ---
  LibraryController libraryController;
  LibraryMetadata& libraryData;
  juce::Image currentArtwork;
  int activeMappingIndex = -1;
  juce::File lastBrowseDirectory;
  juce::File currentLibraryFile;

  void updateOctave(int newOctave);
  void updateGridUI();
  void updateMetadataFromUI();
  void rebuildSynth();
  void auditionSample(const juce::String &path, int midiNote, int velocity);
  void auditionSampleOff(int midiNote);
  void deselectAllRegions();

  using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
  using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

  std::unique_ptr<ComboAtt> compModeAtt;
  std::unique_ptr<SliderAtt> compThreshAtt, compRatioAtt, compAttackAtt, compReleaseAtt;
  std::unique_ptr<ButtonAtt> revEnableAtt;
  std::unique_ptr<SliderAtt> revSizeAtt, revMixAtt, toneAtt, volAtt, pitchAtt;

  juce::CriticalSection synthLock;

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
  SampleRegion* findRegionForIndex(int mappingIndex);
  int dragCounterpartIndex = -1;

  bool dragStickyTop = false;
  bool dragStickyBottom = false;

  juce::MidiMessageCollector midiCollector;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
} // namespace sotero
