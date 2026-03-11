#pragma once

#include "SoteroEngineInterface.h"
#include "SoteroLibraryManager.h"
#include "SoteroUI.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

// --- Shared View Containers ---

namespace sotero {

class SoteroKeyboard : public juce::Component {
public:
  SoteroKeyboard(juce::MidiKeyboardState &state);
  void resized() override;
  void setOctaveRange(int firstNote);

private:
  juce::MidiKeyboardComponent keyboard;
  SoteroArrowButton octLeftBtn{true}, octRightBtn{false};
  int firstNote = 48;
};

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
  sotero::HardwareKnob masterVolKnob, pitchKnob, toneKnob;
  sotero::VUMeter masterVU_L, masterVU_R;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      masterVolAtt, pitchAtt, toneAtt;
};

// ... (LibraryDashboard and SetupView content remains similar but I'll add them
// properly)

class LibraryDashboard : public juce::Component, public juce::Timer {
public:
  LibraryDashboard(sotero::ISoteroAudioEngine &e);
  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

private:
  sotero::ISoteroAudioEngine &engine;
  juce::Label titleLabel, authorLabel;
  juce::ImageComponent artworkComponent;
  juce::TextEditor descriptionDisplay;
  sotero::HardwareKnob volSlider, panSlider;
  sotero::VUMeter vuL, vuR;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volAtt,
      panAtt;
};

class SetupView : public juce::Component {
public:
  SetupView(sotero::ISoteroAudioEngine &e);
  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  sotero::ISoteroAudioEngine &engine;
  juce::Label machineIDLabel, loginLabel, passLabel, serialLabel;
  juce::TextEditor loginInput, passInput, serialInput;
  juce::TextButton activateBtn{"ACTIVATE LICENSE"};
  juce::GroupComponent globalGroup;
  juce::Label chanLabel;
  juce::ComboBox chanCombo;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      chanAtt;

  std::unique_ptr<LibraryDashboard> dashboard;

  juce::TextButton loadLibBtn;
  std::unique_ptr<juce::FileChooser> chooser;
};

class LibraryBrowser : public juce::Component {
public:
  LibraryBrowser(sotero::ISoteroAudioEngine &e);
  void paint(juce::Graphics &g) override;
  void resized() override;

  struct Item : public juce::Component {
    SoteroLibraryManager::LibraryEntry entry;
    std::function<void(juce::File)> onLoadRequested;
    Item(const SoteroLibraryManager::LibraryEntry &e);
    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &) override;
    bool isSelected = false;
  };

  void refresh();

private:
  sotero::ISoteroAudioEngine &engine;
  sotero::SoteroLibraryManager libManager;
  juce::OwnedArray<Item> items;
  juce::Viewport viewport;
  juce::Component content;
};

/**
    Unified Player UI that contains Logo, Navigation, Monitor,
    Performance/Setup views, and the Keyboard.
*/
class SoteroPlayerUI : public juce::Component, public juce::Timer {
public:
  SoteroPlayerUI(sotero::ISoteroAudioEngine &e);
  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

  void setLogo(juce::Image img) { logo.setImage(img); }

private:
  sotero::ISoteroAudioEngine &engine;

  juce::TextButton performBtn{"PERFORMANCE"}, setupBtn{"SETUP"},
      libBtn{"LIBRARY"};
  std::unique_ptr<PerformanceView> performanceView;
  std::unique_ptr<SetupView> setupView;
  std::unique_ptr<LibraryBrowser> libraryBrowser;
  std::unique_ptr<LibraryDashboard> libraryDashboard;

  juce::Label midiMonitorLabel, midiVelocityLabel;
  juce::Label versionLabel{"Version", "v0.4.0"};
  juce::ImageComponent logo;
  SoteroKeyboard keyboard;
};

} // namespace sotero
