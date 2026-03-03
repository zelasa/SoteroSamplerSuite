#pragma once
#include "../../Common/SoteroUI.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

// --- View Containers ---

class PerformanceView : public juce::Component, public juce::Timer {
public:
  PerformanceView(sotero::ISoteroAudioEngine &e);
  ~PerformanceView() override;
  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

private:
  sotero::ISoteroAudioEngine &engine;

  juce::GroupComponent velocityGroup, dynamicsGroup, spatialGroup, masterGroup;

  // Velocity section
  std::unique_ptr<sotero::CurveButton> curveButtons[3];

  // Dynamics (Compressor)
  std::unique_ptr<sotero::ModeButton> compModes[4];
  sotero::HardwareKnob threshKnob, ratioKnob, attackKnob, releaseKnob;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      threshAtt, ratioAtt, attackAtt, releaseAtt;

  // Spatial (Reverb)
  juce::ToggleButton revToggle;
  std::unique_ptr<sotero::ModeButton> revModes[4];
  sotero::HardwareKnob revSizeKnob, revMixKnob;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      revEnableAtt;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      revSizeAtt, revMixAtt;

  // Master
  sotero::HardwareKnob masterVolKnob;
  sotero::VUMeter masterVU_L, masterVU_R;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      masterVolAtt;
};

// (Unified Mixer and Library Info)
class LibraryDashboard : public juce::Component, public juce::Timer {
public:
  LibraryDashboard(sotero::ISoteroAudioEngine &e);
  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

private:
  sotero::ISoteroAudioEngine &engine;
  juce::Label titleLabel, authorLabel;
  juce::Image artwork;
  sotero::HardwareKnob volSlider, panSlider;
  juce::TextButton loadBtn;
  sotero::VUMeter vuL, vuR;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volAtt,
      panAtt;
};

class SetupView : public juce::Component {
public:
  SetupView(sotero::ISoteroAudioEngine &e);
  void resized() override;

private:
  sotero::ISoteroAudioEngine &engine;
  juce::GroupComponent globalGroup;
  juce::Label chanLabel;
  juce::ComboBox chanCombo;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      chanAtt;

  std::unique_ptr<LibraryDashboard> dashboard;

  juce::TextButton loadLibBtn;
  std::unique_ptr<juce::FileChooser> chooser;
};

class SamplerPlayerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::Timer {
public:
  SamplerPlayerAudioProcessorEditor(SamplerPlayerAudioProcessor &);
  ~SamplerPlayerAudioProcessorEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

private:
  SamplerPlayerAudioProcessor &audioProcessor;
  juce::TextButton performBtn, setupBtn;

  std::unique_ptr<PerformanceView> performanceView;
  std::unique_ptr<SetupView> setupView;

  juce::Label midiMonitorLabel;
  juce::Label midiVelocityLabel;
  juce::ImageComponent logo;
  juce::MidiKeyboardComponent keyboardComponent;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      SamplerPlayerAudioProcessorEditor)
};
