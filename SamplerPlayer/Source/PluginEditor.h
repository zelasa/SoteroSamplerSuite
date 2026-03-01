#pragma once

#include "PluginProcessor.h"
#include "VUMeter.h"
#include <JuceHeader.h>

// --- Custom Components ---

class CurveButton : public juce::Button {
public:
  CurveButton(const juce::String &name, int type)
      : juce::Button(name), curveType(type) {
    setClickingTogglesState(true);
    setRadioGroupId(1001);
  }

  void paintButton(juce::Graphics &g, bool, bool) override {
    auto bounds = getLocalBounds().reduced(2);
    auto isSelected = getToggleState();
    g.setColour(isSelected ? juce::Colours::white.withAlpha(0.2f)
                           : juce::Colours::transparentBlack);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    g.setColour(isSelected ? juce::Colours::yellow : juce::Colours::grey);
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, isSelected ? 2.0f : 1.0f);

    auto contentArea = bounds.reduced(5);
    auto iconArea = contentArea.removeFromTop(contentArea.getHeight() * 0.6f);
    g.setColour(isSelected ? juce::Colours::yellow : juce::Colours::white);
    juce::Path p;
    auto pArea = iconArea.toFloat().reduced(4);
    p.startNewSubPath(pArea.getX(), pArea.getBottom());
    if (curveType == 0)
      p.quadraticTo(pArea.getX(), pArea.getY(), pArea.getRight(), pArea.getY());
    else if (curveType == 1)
      p.lineTo(pArea.getRight(), pArea.getY());
    else
      p.quadraticTo(pArea.getRight(), pArea.getBottom(), pArea.getRight(),
                    pArea.getY());
    g.strokePath(p, juce::PathStrokeType(2.5f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(getName(), contentArea, juce::Justification::centred);
  }

private:
  int curveType;
};

// Seletor de modo (Off/Light/Mid/Hard)
class ModeButton : public juce::Button {
public:
  ModeButton(const juce::String &name, int id)
      : juce::Button(name), groupID(id) {
    setClickingTogglesState(true);
    setRadioGroupId(id);
  }

  void paintButton(juce::Graphics &g, bool, bool) override {
    auto bounds = getLocalBounds().reduced(2);
    auto isSelected = getToggleState();
    g.setColour(isSelected ? juce::Colours::orange.withAlpha(0.3f)
                           : juce::Colours::black.withAlpha(0.2f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    g.setColour(isSelected ? juce::Colours::orange : juce::Colours::grey);
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);
    g.setColour(isSelected ? juce::Colours::white : juce::Colours::lightgrey);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(getName(), bounds, juce::Justification::centred);
  }

private:
  int groupID;
};

// Componente Knob customizado (Estilo Hardware)
class HardwareKnob : public juce::Component {
public:
  HardwareKnob(const juce::String &labelText) {
    addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(14.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 22);
    slider.setColour(juce::Slider::rotarySliderFillColourId,
                     juce::Colours::orange);
    slider.setColour(juce::Slider::textBoxOutlineColourId,
                     juce::Colours::transparentBlack);
  }

  void resized() override {
    auto area = getLocalBounds();
    label.setBounds(area.removeFromTop(22));
    slider.setBounds(area);
  }

  juce::Slider &getSlider() { return slider; }

private:
  juce::Label label;
  juce::Slider slider;
};

// --- View Containers ---

class PerformanceView : public juce::Component, public juce::Timer {
public:
  PerformanceView(SamplerPlayerAudioProcessor &p);
  void resized() override;
  void paint(juce::Graphics &g) override;
  void timerCallback() override;

private:
  SamplerPlayerAudioProcessor &processor;

  juce::GroupComponent velocityGroup, dynamicsGroup, spatialGroup, masterGroup;

  // Velocity section
  std::unique_ptr<CurveButton> curveButtons[3];

  // Dynamics (Compressor)
  std::unique_ptr<ModeButton> compModes[4];
  HardwareKnob threshKnob, ratioKnob, attackKnob, releaseKnob;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      threshAtt, ratioAtt, attackAtt, releaseAtt;

  // Spatial (Reverb)
  juce::ToggleButton revToggle;
  std::unique_ptr<ModeButton> revModes[4];
  HardwareKnob revSizeKnob, revMixKnob;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      revEnableAtt;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      revSizeAtt, revMixAtt;

  // Master
  HardwareKnob masterVolKnob;
  VUMeter masterVU_L, masterVU_R;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      masterVolAtt;
};

// (TrackControl remains similar but updated visually)
class TrackControl : public juce::Component, public juce::Timer {
public:
  TrackControl(SamplerPlayerAudioProcessor &p, int index,
               const juce::String &name);
  void resized() override;
  void paint(juce::Graphics &g) override;
  void timerCallback() override;

private:
  SamplerPlayerAudioProcessor &processor;
  int trackIndex;
  juce::Label titleLabel, nameLabel;
  HardwareKnob volSlider, panSlider;
  juce::TextButton loadBtn;
  VUMeter vu;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volAtt,
      panAtt;
};

class SetupView : public juce::Component {
public:
  SetupView(SamplerPlayerAudioProcessor &p);
  void resized() override;

private:
  SamplerPlayerAudioProcessor &processor;
  juce::GroupComponent globalGroup;
  juce::Label chanLabel, noteLabel;
  juce::ComboBox chanCombo, noteCombo;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      chanAtt, noteAtt;

  std::unique_ptr<TrackControl> trackControls[3];
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
