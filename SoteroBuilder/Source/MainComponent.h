#pragma once

#include "../../Common/SoteroEngineInterface.h"
#include "../../Common/SoteroFormat.h"
#include "../../Common/SoteroViews.h"
#include "../../SamplerPlayer/Source/SoteroSamplerVoice.h"
#include "ADSRVisualizer.h"
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

  struct BuilderView : public juce::Component {
    std::function<void()> onBackgroundClick;
    void paint(juce::Graphics &g) override {
      g.fillAll(juce::Colour(0xff121212));
    }
    void mouseDown(const juce::MouseEvent &) override {
      if (onBackgroundClick)
        onBackgroundClick();
    }
  };

  struct KeyColumn : public juce::Component,
                     public juce::FileDragAndDropTarget {
    int noteNumber = 0;
    int colIndex = 0;
    int micLayer = 0;
    juce::OwnedArray<SampleRegion> regions;
    std::function<void()> onBackgroundClick;
    std::function<void(const juce::StringArray &, int)> onFilesDropped;

    KeyColumn(int note, int index, int layer)
        : noteNumber(note), colIndex(index), micLayer(layer) {
      setInterceptsMouseClicks(true, true);
    }

    void addRegion(const KeyMapping &mapping) {
      auto *region = new SampleRegion(mapping, noteNumber - 60, micLayer);
      regions.add(region);
      addAndMakeVisible(region);
      resized();
    }

    void clearRegions() {
      regions.clear();
      resized();
    }

    void paint(juce::Graphics &g) override {
      // Alternating background regardless of black/white keys
      // Professional gray tones: #282828 and #333333
      bool isOdd = (colIndex % 2) != 0;
      g.setColour(isOdd ? juce::Colour(0xff282828) : juce::Colour(0xff333333));
      g.fillAll();

      // Subtle vertical divider
      g.setColour(juce::Colours::white.withAlpha(0.05f));
      g.drawRect(getLocalBounds(), 1.0f);

      // Draw faint velocity reference lines every 16 steps
      g.setColour(juce::Colours::white.withAlpha(0.05f));
      for (int i = 16; i < 127; i += 16) {
        float yPos = getHeight() * (1.0f - (i / 127.0f));
        g.drawLine(0, yPos, (float)getWidth(), yPos, 1.0f);
      }
    }

    void resized() override {
      auto r = getLocalBounds();
      float h = (float)r.getHeight();

      for (auto *region : regions) {
        const auto &mapping = region->getMapping();

        // Map velocity to Y coordinate (0 velocity = bottom, 127 velocity =
        // top)
        float yBottom = h * (1.0f - (mapping.velocityLow / 127.0f));
        float yTop = h * (1.0f - (mapping.velocityHigh / 127.0f));

        float height = yBottom - yTop;

        region->setBounds(0, (int)yTop, getWidth(),
                          (int)juce::jmax(1.0f, height));
      }
    }

    // --- FileDragAndDropTarget Overrides ---
    bool isInterestedInFileDrag(const juce::StringArray &files) override {
      return true;
    }
    void filesDropped(const juce::StringArray &files, int x, int y) override {
      if (onFilesDropped)
        onFilesDropped(files, y);
    }

    void mouseDown(const juce::MouseEvent &) override {
      if (onBackgroundClick)
        onBackgroundClick();
    }
  };

  struct SemiToneKeyboard : public juce::Component {
    std::function<void(int)> onKeyPress;
    std::function<void()> onBackgroundClick;

    void paint(juce::Graphics &g) override {
      auto r = getLocalBounds().toFloat();
      float keyWidth = r.getWidth() / 12.0f;

      for (int i = 0; i < 12; ++i) {
        auto keyRect =
            juce::Rectangle<float>(i * keyWidth, 0, keyWidth, r.getHeight())
                .reduced(0.5f);
        bool isBlack = juce::MidiMessage::isMidiNoteBlack(60 + i);

        if (isBlack) {
          // Background for the key slot
          g.setColour(juce::Colours::white.withAlpha(0.8f));
          g.fillRect(keyRect);

          // The black key "cap" (shorter)
          auto cap =
              keyRect.withHeight(r.getHeight() * 0.65f).reduced(2.0f, 0.0f);
          g.setColour(juce::Colours::black);
          g.fillRoundedRectangle(cap, 2.0f);
        } else {
          g.setColour(juce::Colours::white);
          g.fillRoundedRectangle(keyRect, 2.0f);
        }

        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.drawRoundedRectangle(keyRect, 2.0f, 1.0f);

        // Note name
        g.setColour(isBlack ? juce::Colours::grey : juce::Colours::black);
        g.setFont(10.0f);
        g.drawText(juce::MidiMessage::getMidiNoteName(60 + i, true, false, 3),
                   keyRect.removeFromBottom(15), juce::Justification::centred);
      }
    }

    void mouseDown(const juce::MouseEvent &e) override {
      float keyWidth = (float)getWidth() / 12.0f;
      int note = (int)(e.x / keyWidth);
      if (onKeyPress)
        onKeyPress(60 + note);
      else if (onBackgroundClick)
        onBackgroundClick();
    }
  };

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

  // 1. Header Area
  struct HeaderPanel : public juce::Component {
    juce::Label titleLabel{"Title", "SOTEROPOLYSAMPLES - DEV"};
    juce::TextButton devModeBtn{"DEVELOPER"}, userModeBtn{"USER PLAYER"};
    juce::TextButton saveBtn{"SAVE"}, loadBtn{"LOAD"}, newBtn{"NEW"},
        closeBtn{"CLOSE"};

    // Spec: "TO PLAYER" Master toggle
    juce::ToggleButton toPlayerToggle{"TO PLAYER"};

    juce::Label versionLabel{"Version", "v0.4.0"};

    HeaderPanel() {
      addAndMakeVisible(titleLabel);
      addAndMakeVisible(devModeBtn);
      addAndMakeVisible(userModeBtn);
      addAndMakeVisible(saveBtn);
      addAndMakeVisible(loadBtn);
      addAndMakeVisible(newBtn);
      addAndMakeVisible(closeBtn);
      addAndMakeVisible(toPlayerToggle);
      addAndMakeVisible(versionLabel);

      titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
      titleLabel.setJustificationType(juce::Justification::centred);
      titleLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);

      versionLabel.setFont(juce::Font(14.0f));
      versionLabel.setJustificationType(juce::Justification::centredLeft);
      versionLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

      toPlayerToggle.setColour(juce::ToggleButton::textColourId,
                               juce::Colours::grey);
    }

    void resized() override {
      auto r = getLocalBounds().reduced(5);

      // Upper Right: Mode and Project
      auto rightArea = r.removeFromRight(350);
      auto topRow = rightArea.removeFromTop(25);
      devModeBtn.setBounds(topRow.removeFromLeft(125).reduced(2));
      userModeBtn.setBounds(topRow.reduced(2));

      auto bottomRow = rightArea.reduced(0, 2);
      float btnW = bottomRow.getWidth() / 4.0f;
      saveBtn.setBounds(bottomRow.removeFromLeft(btnW).reduced(2));
      loadBtn.setBounds(bottomRow.removeFromLeft(btnW).reduced(2));
      newBtn.setBounds(bottomRow.removeFromLeft(btnW).reduced(2));
      closeBtn.setBounds(bottomRow.reduced(2));

      // Center-ish: Title
      titleLabel.setBounds(r.removeFromTop(30));

      // Version
      versionLabel.setBounds(r.removeFromLeft(100).reduced(2));

      // Toggle
      toPlayerToggle.setBounds(r.removeFromRight(100).reduced(2));
    }
  } headerPanel;

  // --- Octave Selector ---
  juce::ComboBox octaveSelector;
  int currentOctave = 3; // Default C3

  // --- Sculpting Panel ---
  struct SculptingPanel : public juce::Component {
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::TextEditor attackInput, decayInput, sustainInput, releaseInput;
    juce::Slider cutoffSlider, resSlider, velSensSlider;
    juce::ComboBox filterTypeSelector;
    juce::ToggleButton velToFilterToggle{"ON/OFF"};
    juce::Label title{"Sculpting", "AMP ENVELOPE"};
    juce::Label filterTitle{"Filter", "FILTER TOOLS"};
    ADSRVisualizer adsrVisualizer;

    SculptingPanel() {
      addAndMakeVisible(title);
      addAndMakeVisible(adsrVisualizer);

      addAndMakeVisible(attackSlider);
      addAndMakeVisible(decaySlider);
      addAndMakeVisible(sustainSlider);
      addAndMakeVisible(releaseSlider);

      auto setupEditor = [](juce::TextEditor &ed) {
        ed.setJustification(juce::Justification::centred);
        ed.setColour(juce::TextEditor::backgroundColourId,
                     juce::Colours::transparentWhite);
        ed.setColour(juce::TextEditor::outlineColourId,
                     juce::Colours::yellow.withAlpha(0.2f));
        ed.setColour(juce::TextEditor::focusedOutlineColourId,
                     juce::Colours::yellow);
        ed.setColour(juce::TextEditor::textColourId, juce::Colours::yellow);
        ed.setInputRestrictions(5, "0123456789.");
      };

      setupEditor(attackInput);
      setupEditor(decayInput);
      setupEditor(sustainInput);
      setupEditor(releaseInput);

      addAndMakeVisible(filterTitle);
      addAndMakeVisible(filterTypeSelector);
      filterTypeSelector.addItem("OFF", 1);
      filterTypeSelector.addItem("LP", 2);
      filterTypeSelector.addItem("HP", 3);
      filterTypeSelector.addItem("BP", 4);
      filterTypeSelector.setSelectedId(1, juce::dontSendNotification);

      addAndMakeVisible(cutoffSlider);
      addAndMakeVisible(resSlider);
      addAndMakeVisible(velSensSlider);
      addAndMakeVisible(velToFilterToggle);

      // Spec: yellow graph is a custom draw in paint
      attackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
      attackSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
      decaySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
      decaySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
      sustainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
      sustainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
      releaseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
      releaseSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

      cutoffSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
      resSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
      velSensSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    }
    void resized() override;
    void paint(juce::Graphics &g) override;
  } sculptingPanel;

  // --- Waveform Editor (Spec Dual) ---
  struct WaveformPanel : public juce::Component {
    juce::Colour bgColor;
    juce::String title;
    juce::ToggleButton toPlayerToggle{"TO PLAYER"};

    WaveformPanel(juce::Colour c, juce::String t) : bgColor(c), title(t) {
      addAndMakeVisible(toPlayerToggle);
      toPlayerToggle.setColour(juce::ToggleButton::textColourId,
                               juce::Colours::white);
    }

    void paint(juce::Graphics &g) override;
    void resized() override;
  };

  std::unique_ptr<WaveformPanel> waveform1, waveform2;

  // --- Advanced FX & Loops (Placeholder) ---
  struct AdvancedPanel : public juce::Component {
    juce::Label title{"Advanced", "MASTER FX & DYNAMICS"};

    // Compressor
    juce::GroupComponent compGroup{"", "COMPRESSOR"};
    juce::Slider compThresh, compRatio, compAttack, compRelease;
    juce::ComboBox compMode;

    // Reverb
    juce::GroupComponent revGroup{"", "REVERB"};
    juce::ToggleButton revEnable{"ENABLE"};
    juce::Slider revSize, revMix;

    // Master Tone
    juce::Label toneLabel{"Tone", "TONE"};
    juce::Slider toneSlider;

    AdvancedPanel() {
      addAndMakeVisible(title);
      title.setColour(juce::Label::textColourId, juce::Colours::orange);
      title.setFont(juce::Font(16.0f, juce::Font::bold));

      // Compressor Setup
      addAndMakeVisible(compGroup);
      addAndMakeVisible(compThresh);
      addAndMakeVisible(compRatio);
      addAndMakeVisible(compAttack);
      addAndMakeVisible(compRelease);
      addAndMakeVisible(compMode);
      compMode.addItem("OFF", 1);
      compMode.addItem("CLEAN", 2);
      compMode.addItem("WARM", 3);
      compMode.addItem("PUNCH", 4);
      compMode.setSelectedId(1, juce::dontSendNotification);

      // Reverb Setup
      addAndMakeVisible(revGroup);
      addAndMakeVisible(revEnable);
      addAndMakeVisible(revSize);
      addAndMakeVisible(revMix);

      // Tone
      addAndMakeVisible(toneLabel);
      addAndMakeVisible(toneSlider);

      auto setupSlider = [](juce::Slider &s, juce::String suffix) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        s.setTextValueSuffix(suffix);
      };

      setupSlider(compThresh, " dB");
      setupSlider(compRatio, ":1");
      setupSlider(compAttack, " ms");
      setupSlider(compRelease, " ms");
      setupSlider(revSize, " %");
      setupSlider(revMix, " %");
      setupSlider(toneSlider, "");
      toneSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    }

    void paint(juce::Graphics &g) override {
      auto r = getLocalBounds().reduced(2);
      g.setColour(juce::Colour(0xff0a0a0a));
      g.fillRoundedRectangle(r.toFloat(), 4.0f);
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.drawRoundedRectangle(r.toFloat(), 4.0f, 1.0f);
    }

    void resized() override {
      auto r = getLocalBounds().reduced(10);
      title.setBounds(r.removeFromTop(25));

      auto fxArea = r;
      auto compArea =
          fxArea.removeFromLeft(fxArea.getWidth() * 0.55f).reduced(5);
      compGroup.setBounds(compArea);
      auto c = compArea.reduced(5, 20);
      compMode.setBounds(c.removeFromTop(20));
      float cw = c.getWidth() / 4.0f;
      compThresh.setBounds(c.removeFromLeft(cw));
      compRatio.setBounds(c.removeFromLeft(cw));
      compAttack.setBounds(c.removeFromLeft(cw));
      compRelease.setBounds(c);

      auto revArea = fxArea.removeFromLeft(fxArea.getWidth() * 0.7f).reduced(5);
      revGroup.setBounds(revArea);
      auto rv = revArea.reduced(5, 20);
      revEnable.setBounds(rv.removeFromTop(20));
      float rw = rv.getWidth() / 2.0f;
      revSize.setBounds(rv.removeFromLeft(rw));
      revMix.setBounds(rv);

      auto toneArea = fxArea.reduced(5);
      toneLabel.setBounds(toneArea.removeFromTop(20));
      toneSlider.setBounds(toneArea);
    }
  } advancedPanel;

  // 5. Metadata & Monitoring Panel
  struct MetadataPanel : public juce::Component {
    juce::TextEditor nameEditor, authorEditor, dateEditor, infoEditor;
    juce::Label nameLabel, authorLabel, dateLabel, infoLabel;
    juce::ImageComponent artworkDrop;
    juce::Label artworkLabel{"ArtLabel", "IMAGE DROP"};

    MetadataPanel();
    void resized() override;
    void paint(juce::Graphics &g) override;

    juce::Rectangle<int> vuL, vuR;
    juce::Slider volSlider;
  } metadataPanel;

  // --- Grid Mapper Section ---
  struct LayerSlot : public juce::Component {
    int layerIndex = 0;
    int noteIndex = 0;
    bool hasSample = false;
    juce::String fileName;
    std::function<void()> onFileChanged;
    std::function<void()> onAudition;
    std::function<void()> onClear;

    LayerSlot(int lIdx, int nIdx) : layerIndex(lIdx), noteIndex(nIdx) {}

    void paint(juce::Graphics &g) override {
      auto bounds = getLocalBounds().reduced(1).toFloat();
      g.setColour(hasSample ? juce::Colours::cyan.withAlpha(0.6f)
                            : juce::Colours::white.withAlpha(0.05f));
      g.fillRoundedRectangle(bounds, 3.0f);

      g.setColour(juce::Colours::white.withAlpha(hasSample ? 0.8f : 0.2f));
      g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

      if (hasSample) {
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawFittedText(fileName, getLocalBounds().reduced(2),
                         juce::Justification::centred, 2);
      } else {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.setFont(14.0f);
        g.drawText("+", getLocalBounds(), juce::Justification::centred);
      }
    }

    void mouseDown(const juce::MouseEvent &e) override {
      if (e.mods.isRightButtonDown() || e.mods.isAltDown()) {
        if (onClear)
          onClear();
      } else if (hasSample && onAudition) {
        onAudition();
      } else if (onFileChanged) {
        onFileChanged();
      }
    }
  };

  /**
   * @class SoteroSynthesiser
   * @brief Custom synth that supports Sotero velocity layers.
   */
  class SoteroSynthesiser : public juce::Synthesiser {
  public:
    void noteOn(int midiChannel, int midiNoteNumber, float velocity) override {
      const juce::ScopedLock sl(lock);
      const int v = juce::jlimit(1, 127, (int)(velocity * 127.0f));

      for (auto *sound : sounds) {
        if (sound->appliesToNote(midiNoteNumber) &&
            sound->appliesToChannel(midiChannel)) {
          if (auto *ss =
                  dynamic_cast<const sotero::SoteroSamplerSound *>(sound)) {
            // Apply our custom Velocity check!
            if (ss->appliesToVelocity(v)) {
              // If it passes, ask JUCE to find us a voice (it handles stealing
              // automatically)
              if (auto *voice =
                      findFreeVoice(sound, midiChannel, midiNoteNumber, true)) {
                startVoice(voice, sound, midiChannel, midiNoteNumber, velocity);
              }
            }
          }
        }
      }
    }
  };

  // --- Side-by-Side Mapping Panels ---
  struct MappingPanel : public juce::Component {
    struct LayerView : public juce::Component {
      juce::OwnedArray<KeyColumn> columns;
      std::unique_ptr<SemiToneKeyboard> keyboard;
      juce::Label label;
      juce::ToggleButton toPlayerToggle{"TO PLAYER"};
      juce::Colour themeColor;
      int micLayer;

      LayerView(juce::Colour c, juce::String name, int layerIdx)
          : micLayer(layerIdx), themeColor(c) {
        label.setText(name, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, c);
        addAndMakeVisible(label);
        addAndMakeVisible(toPlayerToggle);

        for (int i = 0; i < 12; ++i)
          columns.add(new KeyColumn(60 + i, i, micLayer));

        for (auto *col : columns)
          addAndMakeVisible(col);

        keyboard = std::make_unique<SemiToneKeyboard>();
        addAndMakeVisible(keyboard.get());
      }

      void resized() override {
        auto r = getLocalBounds();

        // 1. Header (Label + Toggle)
        auto headerArea = r.removeFromTop(25);
        toPlayerToggle.setBounds(headerArea.removeFromRight(100).reduced(2));
        label.setBounds(headerArea.reduced(5, 0));

        // 2. Keyboard (Full width at bottom)
        auto bottomArea = r.removeFromBottom(40);
        keyboard->setBounds(bottomArea);

        // 3. Grid Columns (Remaining area)
        float colW = (float)r.getWidth() / 12.0f;
        for (int i = 0; i < 12; ++i)
          columns[i]->setBounds(juce::roundToInt(i * colW), r.getY(),
                                juce::roundToInt(colW), r.getHeight());
      }
    };

    std::unique_ptr<LayerView> layer1, layer2;
    juce::ComboBox octaveSelector;
    juce::ToggleButton layerSyncLock{"SYNC LAYERS"};
    int currentOctave = 3;

    MappingPanel() {
      layerSyncLock.setTooltip("When ON, moving or resizing a sample in one "
                               "layer will replicate to the other.");
      layerSyncLock.setColour(juce::ToggleButton::textColourId,
                              juce::Colours::orange);
      addAndMakeVisible(layerSyncLock);
      layer1 = std::make_unique<LayerView>(juce::Colours::cyan,
                                           "LAYER 1 (MIC 1)", 0);
      layer2 =
          std::make_unique<LayerView>(juce::Colours::red, "LAYER 2 (MIC 2)", 1);
      addAndMakeVisible(layer1.get());
      addAndMakeVisible(layer2.get());
      addAndMakeVisible(octaveSelector);
      addAndMakeVisible(layerSyncLock);
    }

    void resized() override;

    void updateOctave(int newOctave);
  } mappingPanel;

  // --- Data ---
  LibraryMetadata libraryData;
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

  bool dragStickyTop = false;
  bool dragStickyBottom = false;

  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
} // namespace sotero
