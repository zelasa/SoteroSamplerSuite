#pragma once

#include "../../Common/SoteroFormat.h"
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
                      public juce::MidiInputCallback {
public:
  MainComponent();
  ~MainComponent() override;

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

private:
  std::unique_ptr<juce::FileChooser> chooser;
  // --- Library Info Section ---
  juce::Label titleLabel{"BuilderTitle", "SOTERO BUILDER"};
  juce::TextEditor nameEditor;
  juce::TextEditor authorEditor;

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
      const juce::ScopedLock sl(
          lock); // Lock for thread-safety during iteration
      const int v = juce::jlimit(1, 127, (int)(velocity * 127.0f));
      for (auto *sound : sounds) {
        if (sound->appliesToNote(midiNoteNumber) &&
            sound->appliesToChannel(midiChannel)) {
          if (auto *ss =
                  dynamic_cast<const sotero::SoteroSamplerSound *>(sound)) {
            if (ss->appliesToVelocity(v)) {
              for (auto *voice : voices) {
                if (voice->canPlaySound(sound)) {
                  if (!voice->isVoiceActive()) {
                    startVoice(voice, sound, midiChannel, midiNoteNumber,
                               velocity);
                    return;
                  }
                }
              }
            }
          }
        }
      }
    }
  };

  struct KeyColumn : public juce::Component {
    juce::OwnedArray<SampleRegion> regions;
    int noteNumber = 60;

    KeyColumn(int note) : noteNumber(note) {}

    void addRegion(const KeyMapping &mapping) {
      auto *region = new SampleRegion(mapping, noteNumber - 60);
      regions.add(region);
      addAndMakeVisible(region);
      resized();
    }

    void clearRegions() {
      regions.clear();
      resized();
    }

    void paint(juce::Graphics &g) override {
      bool isBlack = juce::MidiMessage::isMidiNoteBlack(noteNumber);
      g.setColour(isBlack ? juce::Colours::white.withAlpha(0.02f)
                          : juce::Colours::transparentWhite);
      g.fillAll();

      if (isBlack) {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawRect(getLocalBounds(), 1.0f);
      }

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

        region->setBounds(0, (int)yTop, getWidth(), (int)height);
      }
    }
  };

  struct SemiToneKeyboard : public juce::Component {
    std::function<void(int)> onKeyPress;

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
        const juce::String noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                          "F#", "G",  "G#", "A",  "A#", "B"};
        g.drawText(noteNames[i], keyRect.removeFromBottom(20),
                   juce::Justification::centred);
      }
    }

    void mouseDown(const juce::MouseEvent &e) override {
      if (onKeyPress) {
        int keyIndex = (int)(e.x / (getWidth() / 12.0f));
        onKeyPress(60 + keyIndex);
      }
    }
  };

  juce::OwnedArray<KeyColumn> keyColumns;
  std::unique_ptr<SemiToneKeyboard> keyboard;

  // --- Data ---
  LibraryMetadata libraryData;
  juce::File lastBrowseDirectory;
  juce::File currentLibraryFile;

  void updateGridUI();
  void updateMetadataFromUI();
  void rebuildSynth();
  void auditionSample(const juce::String &path, int midiNote, int velocity);

  // --- Actions ---
  juce::TextButton exportButton{"GENERATE .SPSA"};
  juce::TextButton importButton{"IMPORT .SPSA"};

  // --- Audio ---
  juce::AudioFormatManager formatManager;
  SoteroSynthesiser synth;
  juce::CriticalSection synthLock;
  juce::MidiBuffer emptyMidi;
  juce::MidiMessageCollector midiCollector;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
} // namespace sotero
