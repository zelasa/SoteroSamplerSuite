#include "MainComponent.h"
#include "../../Common/SoteroArchive.h"
#include "../../SamplerPlayer/Source/SoteroSamplerVoice.h"
#include "BinaryData.h"
#include <thread>
#include "../../Common/UI/LoopMainControls.h"

namespace sotero {

// MainComponent implementation
MainComponent::MainComponent()
    : engine(std::make_unique<SoteroEngine>()),
      libraryData(engine->getMetadata()) {
  setLookAndFeel(&lookAndFeel);
  libraryData.mappings.clear();

  // --- BUILDERUI Modular Panels ---
  addAndMakeVisible(headerPanel);
  headerPanel.getLoopsBtn().onClick = [this] {
      auto* popup = new LoopMainControls(*engine);
      popup->setSize(400, 300);
      juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(popup), 
                                             headerPanel.getLoopsBtn().getScreenBounds(), 
                                             nullptr);
  };

  waveform1 = std::make_unique<WaveformWidget>(0);
  waveform2 = std::make_unique<WaveformWidget>(1);
  addAndMakeVisible(waveform1.get());
  addAndMakeVisible(waveform2.get());

  addAndMakeVisible(sculptingPanel);
  sculptingPanel.setEnabled(false);
  addAndMakeVisible(mappingPanel);
  addAndMakeVisible(metadataPanel);
  addAndMakeVisible(advancedPanel);

  startTimerHz(30); // Poll for playhead at 30Hz

  // --- Sculpting Panel Wiring ---
  auto &sp = sculptingPanel;

  sp.getADSR().onADSRChanged = [this](float a, float d, float s, float r, float sTime) {
    if (activeMappingIndex >= 0 && activeMappingIndex < libraryData.mappings.size()) {
        auto &m = libraryData.mappings.getReference(activeMappingIndex);
        m.adsrAttack = a;
        m.adsrDecay = d;
        m.adsrSustain = s;
        m.adsrRelease = r;
        m.adsrSustainTime = sTime;
        
        engine->updateSoundParameters(activeMappingIndex, m);

        // Sync Logic
        if (mappingPanel.layerSyncLock.getToggleState()) {
            int counterpart = findCounterpart(activeMappingIndex);
            if (counterpart >= 0) {
                auto &mc = libraryData.mappings.getReference(counterpart);
                mc.adsrAttack = a;
                mc.adsrDecay = d;
                mc.adsrSustain = s;
                mc.adsrRelease = r;
                mc.adsrSustainTime = sTime;
                engine->updateSoundParameters(counterpart, mc);
            }
        }
    }
  };

  sp.getFilter().onFilterChanged = [this](int type, float cutoff, float res) {
    if (activeMappingIndex >= 0 && activeMappingIndex < libraryData.mappings.size()) {
        auto &m = libraryData.mappings.getReference(activeMappingIndex);
        m.filterType = type;
        m.filterCutoff = cutoff;
        m.filterResonance = res;
        
        engine->updateSoundParameters(activeMappingIndex, m);

        if (mappingPanel.layerSyncLock.getToggleState()) {
            int counterpart = findCounterpart(activeMappingIndex);
            if (counterpart >= 0) {
                auto &mc = libraryData.mappings.getReference(counterpart);
                mc.filterType = type;
                mc.filterCutoff = cutoff;
                mc.filterResonance = res;
                engine->updateSoundParameters(counterpart, mc);
            }
        }
    }
  };

  // ADSR Slope/Visualizer specialized callbacks
  sp.getADSR().getVisualizer().onAttackCurveChange = [this](float val) {
    if (activeMappingIndex >= 0 && activeMappingIndex < libraryData.mappings.size()) {
      auto& m = libraryData.mappings.getReference(activeMappingIndex);
      m.adsrAttackCurve = val;
      engine->updateSoundParameters(activeMappingIndex, m);

      if (mappingPanel.layerSyncLock.getToggleState()) {
          int cp = findCounterpart(activeMappingIndex);
          if (cp >= 0) {
              auto& mc = libraryData.mappings.getReference(cp);
              mc.adsrAttackCurve = val;
              engine->updateSoundParameters(cp, mc);
          }
      }
    }
  };
  sp.getADSR().getVisualizer().onDecayCurveChange = [this](float val) {
    if (activeMappingIndex >= 0 && activeMappingIndex < libraryData.mappings.size()) {
      auto& m = libraryData.mappings.getReference(activeMappingIndex);
      m.adsrDecayCurve = val;
      engine->updateSoundParameters(activeMappingIndex, m);

      if (mappingPanel.layerSyncLock.getToggleState()) {
          int cp = findCounterpart(activeMappingIndex);
          if (cp >= 0) {
              auto& mc = libraryData.mappings.getReference(cp);
              mc.adsrDecayCurve = val;
              engine->updateSoundParameters(cp, mc);
          }
      }
    }
  };
  sp.getADSR().getVisualizer().onReleaseCurveChange = [this](float val) {
    if (activeMappingIndex >= 0 && activeMappingIndex < libraryData.mappings.size()) {
      auto& m = libraryData.mappings.getReference(activeMappingIndex);
      m.adsrReleaseCurve = val;
      engine->updateSoundParameters(activeMappingIndex, m);

      if (mappingPanel.layerSyncLock.getToggleState()) {
          int cp = findCounterpart(activeMappingIndex);
          if (cp >= 0) {
              auto& mc = libraryData.mappings.getReference(cp);
              mc.adsrReleaseCurve = val;
              engine->updateSoundParameters(cp, mc);
          }
      }
    }
  };
  sp.getADSR().getVisualizer().onSustainTimeChange = [this](float val) {
    if (activeMappingIndex >= 0 && activeMappingIndex < libraryData.mappings.size()) {
      auto& m = libraryData.mappings.getReference(activeMappingIndex);
      m.adsrSustainTime = val;
      engine->updateSoundParameters(activeMappingIndex, m);

      if (mappingPanel.layerSyncLock.getToggleState()) {
          int cp = findCounterpart(activeMappingIndex);
          if (cp >= 0) {
              auto& mc = libraryData.mappings.getReference(cp);
              mc.adsrSustainTime = val;
              engine->updateSoundParameters(cp, mc);
          }
      }
    }
  };

  // --- Project Controls Setup ---
  headerPanel.getLoadBtn().onClick = [this] {
    chooser = std::make_unique<juce::FileChooser>(
        "Select a Sotero Library...",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.sotero");

    auto flags = juce::FileBrowserComponent::openMode |
                 juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(flags, [this](const juce::FileChooser &fc) {
      auto result = fc.getResult();
      if (result.existsAsFile())
        loadSoteroLibrary(result);
    });
  };

  headerPanel.getSaveBtn().onClick = [this] {
    updateMetadataFromUI();
    chooser = std::make_unique<juce::FileChooser>(
        "Save Sotero Library...",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile(libraryData.name + ".spsa"),
        "*.spsa;*.sotero");

    auto flags = juce::FileBrowserComponent::saveMode |
                 juce::FileBrowserComponent::warnAboutOverwriting;
    chooser->launchAsync(flags, [this](const juce::FileChooser &fc) {
      auto result = fc.getResult();
      if (result != juce::File{})
        sotero::SoteroArchive::write(result, libraryData, currentLibraryFile);
    });
  };

  headerPanel.getNewBtn().onClick = [this] {
    deselectAllRegions();
    libraryData = LibraryMetadata();
    libraryData.mappings.clear();
    updateGridUI();
  };

  // --- Octave / Mapping Setup ---
  for (int i = -2; i <= 8; ++i) {
    mappingPanel.octaveSelector.addItem("Octave C" + juce::String(i), i + 3);
  }
  mappingPanel.octaveSelector.setSelectedId(mappingPanel.currentOctave + 3);
  mappingPanel.octaveSelector.onChange = [this] {
    updateOctave(mappingPanel.octaveSelector.getSelectedId() - 3);
  };
  mappingPanel.layerSyncLock.onClick = [this] {
    if (mappingPanel.layerSyncLock.getToggleState()) {
      alignLayers();
    }
  };

  // --- Mapping / KeyColumn Callbacks ---
  auto setupCol = [this](KeyColumn *col) {
    col->onFilesDropped = [this, col](const juce::StringArray &files,
                                      int localY) {
      if (files.size() == 0)
        return;

      float dropVelocityNormalized =
          1.0f - ((float)localY / (float)col->getHeight());
      int targetVelocity =
          juce::jlimit(0, 127, (int)(dropVelocityNormalized * 127.0f));

      int span = 20;
      int velLow = juce::jlimit(0, 127, targetVelocity - span / 2);
      int velHigh = juce::jlimit(0, 127, targetVelocity + span / 2);

      for (int i = 0; i < files.size(); ++i) {
        KeyMapping m;
        m.midiNote = col->noteNumber;
        m.micLayer = col->micLayer;
        m.samplePath = files[i];
        m.fileName = juce::File(files[i]).getFileName();
        m.velocityLow = juce::jlimit(0, 126, velLow);
        m.velocityHigh = juce::jlimit(m.velocityLow + 1, 127, velHigh);

        // --- PREVENT STACKING ---
        // Replace existing mappings on this note/layer that overlap this
        // velocity zone
        for (int j = libraryData.mappings.size() - 1; j >= 0; --j) {
          auto &existing = libraryData.mappings.getReference(j);
          if (existing.midiNote == m.midiNote &&
              existing.micLayer == m.micLayer) {
            if (existing.velocityLow <= m.velocityHigh &&
                existing.velocityHigh >= m.velocityLow) {
              existing.samplePath = ""; // Mark for removal in updateGridUI
            }
          }
        }
        libraryData.mappings.add(m);

        velHigh = velLow - 1;
        velLow = juce::jlimit(0, 127, velHigh - span);
      }

      updateGridUI();
      rebuildSynth();
    };
  };

  if (mappingPanel.layer1) {
    for (auto *col : mappingPanel.layer1->columns)
      setupCol(col);
  }
  if (mappingPanel.layer2) {
    for (auto *col : mappingPanel.layer2->columns)
      setupCol(col);
  }

  updateGridUI();

  if (mappingPanel.layer1 && mappingPanel.layer1->keyboard) {
    mappingPanel.layer1->keyboard->onKeyPress = [this](int note) {
      engine->getSynth().noteOn(1, note + (mappingPanel.currentOctave * 12), 0.8f);
    };
  }
  if (mappingPanel.layer2 && mappingPanel.layer2->keyboard) {
    mappingPanel.layer2->keyboard->onKeyPress = [this](int note) {
      engine->getSynth().noteOn(1, note + (mappingPanel.currentOctave * 12), 0.8f);
    };
  }

  // --- TO PLAYER Toggles Wiring (Independent Layers) ---
  mappingPanel.layer1->toPlayerToggle.onStateChange = [this] {
    bool state = mappingPanel.layer1->toPlayerToggle.getToggleState();
    if (waveform1) waveform1->getToPlayerToggle().setToggleState(state, juce::dontSendNotification);
    engine->setLayerBypass(0, !state);
  };
  mappingPanel.layer2->toPlayerToggle.onStateChange = [this] {
    bool state = mappingPanel.layer2->toPlayerToggle.getToggleState();
    if (waveform2) waveform2->getToPlayerToggle().setToggleState(state, juce::dontSendNotification);
    engine->setLayerBypass(1, !state);
  };

  if (waveform1) {
      waveform1->getToPlayerToggle().onStateChange = [this] {
          mappingPanel.layer1->toPlayerToggle.setToggleState(waveform1->getToPlayerToggle().getToggleState(), juce::sendNotification);
      };
  }
  if (waveform2) {
      waveform2->getToPlayerToggle().onStateChange = [this] {
          mappingPanel.layer2->toPlayerToggle.setToggleState(waveform2->getToPlayerToggle().getToggleState(), juce::sendNotification);
      };
  }

  headerPanel.getToPlayerToggle().onStateChange = [this] {
    bool state = headerPanel.getToPlayerToggle().getToggleState();
    mappingPanel.layer1->toPlayerToggle.setToggleState(state, juce::sendNotification);
    mappingPanel.layer2->toPlayerToggle.setToggleState(state, juce::sendNotification);
  };

  // Initial Sync: UI starts unchecked, Engine now starts bypassed=true.
  // Explicitly trigger a refresh just in case.
  engine->setLayerBypass(0, true);
  engine->setLayerBypass(1, true);

  setAudioChannels(0, 2);
  lastBrowseDirectory =
      juce::File::getSpecialLocation(juce::File::userHomeDirectory);
  setSize(1100, 850);

  // --- Master FX Wiring ---
  auto &ap = engine->getAPVTS();
  compModeAtt =
      std::make_unique<ComboAtt>(ap, "masterComp", advancedPanel.getDynamics().getModeSelector());
  compThreshAtt =
      std::make_unique<SliderAtt>(ap, "compThresh", advancedPanel.getDynamics().getThresholdSlider());
  compRatioAtt =
      std::make_unique<SliderAtt>(ap, "compRatio", advancedPanel.getDynamics().getRatioSlider());
  compAttackAtt =
      std::make_unique<SliderAtt>(ap, "compAttack", advancedPanel.getDynamics().getAttackSlider());
  compReleaseAtt =
      std::make_unique<SliderAtt>(ap, "compRelease", advancedPanel.getDynamics().getReleaseSlider());

  revEnableAtt =
      std::make_unique<ButtonAtt>(ap, "revEnable", advancedPanel.getRevEnable());
  revSizeAtt =
      std::make_unique<SliderAtt>(ap, "revSize", advancedPanel.getRevSize());
  revMixAtt = std::make_unique<SliderAtt>(ap, "revMix", advancedPanel.getRevMix());

  toneAtt =
      std::make_unique<SliderAtt>(ap, "masterTone", advancedPanel.getToneSlider());
  volAtt = std::make_unique<SliderAtt>(
      ap, "masterVol", metadataPanel.getVolSlider()); // Assuming added or reuse
  pitchAtt = std::make_unique<SliderAtt>(
      ap, "masterPitch",
      sculptingPanel.getVelSensSlider()); // Reusing for now or adding

  // --- Mode Button Logic ---
  headerPanel.getDevModeBtn().onClick = [this] { setUIMode(UIMode::Developer); };
  headerPanel.getUserModeBtn().onClick = [this] { setUIMode(UIMode::UserPlayer); };

  updateGridUI();
}

void MainComponent::setUIMode(UIMode mode) {
  currentUIMode = mode;

  bool isDev = (mode == UIMode::Developer);

  // Toggle visibility of Builder-centric panels
  mappingPanel.setVisible(isDev);
  sculptingPanel.setVisible(isDev);
  if (waveform1)
    waveform1->setVisible(isDev);
  if (waveform2)
    waveform2->setVisible(isDev);

  // Advanced Panel and Metadata are visible in BOTH, but resized differently
  // or we could show a distinct PerformanceView later. For now, we adjust
  // layout.

  headerPanel.getDevModeBtn().setToggleState(isDev, juce::dontSendNotification);
  headerPanel.getUserModeBtn().setToggleState(!isDev, juce::dontSendNotification);

  resized();
}

MainComponent::~MainComponent() {
  setLookAndFeel(nullptr);
  deviceManager.removeMidiInputDeviceCallback({}, this);
  shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  midiCollector.reset(sampleRate);
  engine->prepare(sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  bufferToFill.clearActiveBufferRegion();

  juce::MidiBuffer incomingMidi;
  midiCollector.removeNextBlockOfMessages(incomingMidi,
                                          bufferToFill.numSamples);

  // We still use the collector here, but let the engine do the heavy lifting
  engine->process(*bufferToFill.buffer, incomingMidi);
}

void MainComponent::handleIncomingMidiMessage(
    juce::MidiInput *source, const juce::MidiMessage &message) {
  midiCollector.addMessageToQueue(message);
}

void MainComponent::releaseResources() {}

void MainComponent::rebuildSynth() {
  engine->rebuildSynth();
}

void MainComponent::auditionSample(const juce::String &path, int midiNote,
                                   int velocity) {
  // Synth note on will trigger the sounds already loaded in rebuildSynth
  engine->getSynth().noteOn(1, midiNote, (float)velocity / 127.0f);
}

void MainComponent::auditionSampleOff(int midiNote) {
  // Synth note off ensures the ADSR enters release phase and frees polyphony
  engine->getSynth().noteOff(1, midiNote, 0.0f, true);
}

void MainComponent::updateGridUI() {
  if (mappingPanel.layer1) {
    for (auto *col : mappingPanel.layer1->columns)
      col->clearRegions();
  }
  if (mappingPanel.layer2) {
    for (auto *col : mappingPanel.layer2->columns)
      col->clearRegions();
  }

  for (int mIndex = 0; mIndex < libraryData.mappings.size(); ++mIndex) {
    auto &mapping = libraryData.mappings.getReference(mIndex);
    if (mapping.samplePath.isEmpty())
      continue;

    int baseNote = (mappingPanel.currentOctave + 3) * 12;
    int colIndex = mapping.midiNote - baseNote;

    if (colIndex >= 0 && colIndex < 12) {
      auto *col = (mapping.micLayer == 0 && mappingPanel.layer1) ? mappingPanel.layer1->columns[colIndex]
                  : (mapping.micLayer == 1 && mappingPanel.layer2) ? mappingPanel.layer2->columns[colIndex] : nullptr;

      if (col == nullptr)
        continue;

      col->addRegion(mapping, mIndex);
      auto *region = col->regions.getLast();

      region->onAudition = [this, mIndex](const KeyMapping &m, float clickNormalY) {
        // Map click Y position (0=bottom=soft, 1=top=loud) to the sample's velocity range
        float velLow  = m.velocityLow  / 127.0f;
        float velHigh = m.velocityHigh / 127.0f;
        float velocity = juce::jlimit(velLow, velHigh, 
                                      velLow + clickNormalY * (velHigh - velLow));
        engine->auditionMappingStart(mIndex, velocity);
      };
      region->onAuditionEnd = [this, mIndex](const KeyMapping &m) {
        engine->auditionMappingStop(mIndex);
      };

      region->onDragStart = [this, mIndex](bool stickyTop, bool stickyBottom) {
        dragStickyTop = stickyTop;
        dragStickyBottom = stickyBottom;
        
        bool syncActive = mappingPanel.layerSyncLock.getToggleState();
        if (syncActive) {
            dragCounterpartIndex = findCounterpart(mIndex);
        } else {
            dragCounterpartIndex = -1;
        }
      };

      if (mIndex == activeMappingIndex)
        region->setActive(true);

      region->onSelect = [this, mIndex]() {
        // Deselect others visually without destroying everything
        activeMappingIndex = mIndex;
        for (auto *l : {mappingPanel.layer1.get(), mappingPanel.layer2.get()})
          if (l)
            for (auto *c : l->columns)
              for (auto *r : c->regions)
                r->setActive(false);

        sculptingPanel.setEnabled(true);
        auto &m = libraryData.mappings.getReference(mIndex);

        sculptingPanel.getADSR().setParams(
            m.adsrAttack, m.adsrDecay, m.adsrSustain, m.adsrRelease,
            m.adsrAttackCurve, m.adsrDecayCurve, m.adsrReleaseCurve, 1.0f,
            m.adsrSustainTime);

        sculptingPanel.getFilter().setParams(m.filterType, m.filterCutoff, m.filterResonance);
        // Do NOT call updateGridUI() here as it destroys the very sliders we
        // are using! Instead, just repaint the regions to show the active state
        // if needed
        for (auto *l : {mappingPanel.layer1.get(), mappingPanel.layer2.get()})
          if (l)
            for (auto *c : l->columns)
              for (auto *r : c->regions)
                r->repaint();
      };

      region->onErase = [this, mIndex]() {
        if (mIndex >= 0 && mIndex < libraryData.mappings.size()) {
          auto &orig = libraryData.mappings.getReference(mIndex);
          int oldNote = orig.midiNote;
          int oldLo = orig.velocityLow;
          int oldHi = orig.velocityHigh;
          orig.samplePath = "";

          if (mappingPanel.layerSyncLock.getToggleState()) {
            int otherLayer = 1 - orig.micLayer;
            for (int j = 0; j < libraryData.mappings.size(); ++j) {
              auto &other = libraryData.mappings.getReference(j);
              if (other.micLayer == otherLayer && other.midiNote == oldNote &&
                  other.velocityLow == oldLo && other.velocityHigh == oldHi &&
                  other.samplePath.isNotEmpty()) {
                other.samplePath = "";
                break;
              }
            }
          }
        }
      };



      region->onClear = [this, mIndex](const KeyMapping &) {
        if (mIndex >= 0 && mIndex < libraryData.mappings.size()) {
          libraryData.mappings.getReference(mIndex).samplePath = "";
          // Async call to avoid destroying 'this' inside its own event handler
          juce::MessageManager::callAsync([this] {
            updateGridUI();
            rebuildSynth();
          });
        }
      };

      juce::Component::SafePointer<KeyColumn> safeCol(col);
      region->onBoundsChanged = [this, mIndex, safeCol,
                                 region](const KeyMapping &m) {
        if (mIndex < 0 || mIndex >= libraryData.mappings.size()) return;

        auto &ref    = libraryData.mappings.getReference(mIndex);
        const bool altDown = (region != nullptr) && region->getIsAltDrag();

        // --- Compute deltas so we can propagate them to linked neighbours ---
        const int oldLo  = ref.velocityLow;
        const int oldHi  = ref.velocityHigh;
        const int deltaLo = m.velocityLow  - oldLo;  // BottomHandle moved
        const int deltaHi = m.velocityHigh - oldHi;  // TopHandle moved

        bool syncActive = mappingPanel.layerSyncLock.getToggleState();
        int  otherIndex = syncActive ? findCounterpart(mIndex) : -1;

        // ---------------------------------------------------------------
        // BI-DIRECTIONAL EDGE BINDING
        // Without Alt: if an edge (top or bottom) is touching a neighbour's
        // opposite edge, propagate the same delta to that neighbour.
        // ---------------------------------------------------------------
        if (!altDown) {
          for (int i = 0; i < libraryData.mappings.size(); ++i) {
            if (i == mIndex || i == otherIndex) continue;
            auto &nb = libraryData.mappings.getReference(i);
            if (nb.samplePath.isEmpty() || nb.midiNote != ref.midiNote || nb.micLayer != ref.micLayer)
              continue;

            auto applyToNeighbor = [&](int nbIdx, int dLo, int dHi) {
                auto& n = libraryData.mappings.getReference(nbIdx);
                bool nChanged = false;
                if (dLo != 0 && n.velocityHigh == oldLo - 1) {
                    n.velocityHigh = juce::jlimit(n.velocityLow + 1, 127, n.velocityHigh + dLo);
                    nChanged = true;
                }
                if (dHi != 0 && n.velocityLow == oldHi + 1) {
                    n.velocityLow = juce::jlimit(0, n.velocityHigh - 1, n.velocityLow + dHi);
                    nChanged = true;
                }
                if (nChanged) {
                    if (auto* nbRegion = findRegionForIndex(nbIdx))
                        nbRegion->updateFromMapping(n);
                }
                return nChanged;
            };

            if (applyToNeighbor(i, deltaLo, deltaHi)) {
                // If neighbor changed and sync is on, update its counterpart too
                if (syncActive) {
                    int nbOtherIdx = findCounterpart(i);
                    if (nbOtherIdx != -1) {
                        auto& nbOther = libraryData.mappings.getReference(nbOtherIdx);
                        nbOther.velocityLow = nb.velocityLow;
                        nbOther.velocityHigh = nb.velocityHigh;
                        if (auto* nbOtherRegion = findRegionForIndex(nbOtherIdx))
                            nbOtherRegion->updateFromMapping(nbOther);
                    }
                }
            }
          }
        }

        // ---------------------------------------------------------------
        // PRIMARY MAPPING UPDATE (with collision resolution)
        // ---------------------------------------------------------------
        int targetLo = m.velocityLow;
        int targetHi = m.velocityHigh;

        resolveCollisions(ref.midiNote, ref.micLayer, targetLo, targetHi,
                          mIndex, true, true);

        if (syncActive && otherIndex != -1) {
          auto &other          = libraryData.mappings.getReference(otherIndex);
          other.velocityLow    = targetLo;
          other.velocityHigh   = targetHi;
          resolveCollisions(other.midiNote, other.micLayer, other.velocityLow,
                            other.velocityHigh, otherIndex, true, false);
          // If other layer pushed back, honour that in the primary too
          targetLo = other.velocityLow;
          targetHi = other.velocityHigh;
        }

        ref.velocityLow  = targetLo;
        ref.velocityHigh = targetHi;

        region->updateFromMapping(ref);

        updateColumnRegions(ref.midiNote, ref.micLayer);
        if (syncActive && otherIndex != -1) {
          auto &other = libraryData.mappings.getReference(otherIndex);
          updateColumnRegions(other.midiNote, other.micLayer);
        }
      };

      region->onRequestMove = [this, mIndex, region](int deltaNote, bool altDown) {
        if (mIndex < 0 || mIndex >= libraryData.mappings.size()) return;
        if (deltaNote == 0) return;

        auto& ref = libraryData.mappings.getReference(mIndex);
        const int oldNote = ref.midiNote;
        const int targetNote = juce::jlimit(0, 127, oldNote + deltaNote);
        if (targetNote == oldNote) return;

        // Lambda to transfer a region between columns without destroying it
        auto transferInPlace = [&](int mIdx, SampleRegion* reg, MappingWidget::LayerView* layerView, int oldN, int newN) -> bool {
            int baseNote = (mappingPanel.currentOctave + 3) * 12;
            int oldColIdx = oldN - baseNote;
            int newColIdx = newN - baseNote;
            if (oldColIdx < 0 || oldColIdx >= 12 || newColIdx < 0 || newColIdx >= 12) return false;
            if (!layerView) return false;

            KeyColumn* oldCol = layerView->columns[oldColIdx];
            KeyColumn* newCol = layerView->columns[newColIdx];
            if (!oldCol || !newCol) return false;

            // Find index in OwnedArray
            int rank = -1;
            for (int k = 0; k < oldCol->regions.size(); ++k) {
                if (oldCol->regions[k] == reg) { rank = k; break; }
            }
            if (rank == -1) return false;

            // Update mapping data note immediately
            libraryData.mappings.getReference(mIdx).midiNote = newN;

            // ATOMIC TRANSFER: Remove from old, add to new
            SampleRegion* detached = oldCol->regions.removeAndReturn(rank);
            if (detached) {
                newCol->regions.add(detached);
                newCol->addAndMakeVisible(detached);
                oldCol->resized();
                newCol->resized();
                return true;
            }
            return false;
        };

        bool syncActive = mappingPanel.layerSyncLock.getToggleState();
        int otherIndex = (syncActive && dragCounterpartIndex != -1) ? dragCounterpartIndex : -1;
        MappingWidget::LayerView* primaryLayer = (ref.micLayer == 0) ? mappingPanel.layer1.get() : mappingPanel.layer2.get();

        bool moved = transferInPlace(mIndex, region, primaryLayer, oldNote, targetNote);

        if (moved && syncActive && otherIndex != -1) {
            auto& other = libraryData.mappings.getReference(otherIndex);
            int otherOld = other.midiNote;
            MappingWidget::LayerView* otherLayer = (other.micLayer == 0) ? mappingPanel.layer1.get() : mappingPanel.layer2.get();
            if (auto* otherReg = findRegionForIndex(otherIndex)) {
                transferInPlace(otherIndex, otherReg, otherLayer, otherOld, targetNote);
            }
        }

        if (moved) {
            juce::MessageManager::callAsync([this] { rebuildSynth(); });
        }
      };

      region->onDragFinished = [this](const KeyMapping &) {
        dragStickyTop = false;
        dragStickyBottom = false;
        dragCounterpartIndex = -1;
        juce::MessageManager::callAsync([this] {
          updateGridUI();
          rebuildSynth();
        });
      };

      // ── Vertical Swap (Alt+Body drag) ──────────────────────────────────────
      region->onVerticalSwapRequest = [this, mIndex, region](int deltaVelCenter, int mouseScreenY) {
        if (mIndex < 0 || mIndex >= libraryData.mappings.size())
          return;

        // --- MULTI-SWAP LOOP ---
        bool swapped = true;
        while (swapped) {
            swapped = false;
            auto &src = libraryData.mappings.getReference(mIndex);
            int srcCentre = (src.velocityLow + src.velocityHigh) / 2;
            int targetCentre = srcCentre + deltaVelCenter;

            // Find neighbors ... (rest of logic remains same)
            int neighborIndex = -1;
            if (deltaVelCenter > 0) { // Moving UP
                int bestNextLow = 129;
                for (int i = 0; i < libraryData.mappings.size(); ++i) {
                    if (i == mIndex) continue;
                    const auto &m = libraryData.mappings.getReference(i);
                    if (m.samplePath.isEmpty() || m.midiNote != src.midiNote || m.micLayer != src.micLayer) continue;
                    if (m.velocityLow > src.velocityHigh && m.velocityLow < bestNextLow) {
                        bestNextLow = m.velocityLow;
                        neighborIndex = i;
                    }
                }
            } else if (deltaVelCenter < 0) { // Moving DOWN
                int bestNextHigh = -1;
                for (int i = 0; i < libraryData.mappings.size(); ++i) {
                    if (i == mIndex) continue;
                    const auto &m = libraryData.mappings.getReference(i);
                    if (m.samplePath.isEmpty() || m.midiNote != src.midiNote || m.micLayer != src.micLayer) continue;
                    if (m.velocityHigh < src.velocityLow && m.velocityHigh > bestNextHigh) {
                        bestNextHigh = m.velocityHigh;
                        neighborIndex = i;
                    }
                }
            }

            if (neighborIndex != -1) {
                const auto &nb = libraryData.mappings.getReference(neighborIndex);
                int nbCentre = (nb.velocityLow + nb.velocityHigh) / 2;
                
                // PIXEL-BASED HYSTERESIS
                // We calculate how many velocity units represent ~6 pixels on screen.
                int baseNote = (mappingPanel.currentOctave + 3) * 12;
                int colIndex = src.midiNote - baseNote;
                auto* layerView = (src.micLayer == 0) ? mappingPanel.layer1.get() : mappingPanel.layer2.get();
                float colH = (layerView && layerView->columns[colIndex]) ? (float)layerView->columns[colIndex]->getHeight() : (float)getHeight();
                
                float velPerPixel = 127.0f / colH;
                float hysteresisVel = 15.0f * velPerPixel; // Increased to 15 pixels for "heavy/magnetic" feel

                int midPoint = (srcCentre + nbCentre) / 2;

                bool shouldSwap = false;
                if (deltaVelCenter > 0) { // Moving UP
                    if ((float)targetCentre > (float)midPoint + hysteresisVel) shouldSwap = true;
                } else { // Moving DOWN
                    if ((float)targetCentre < (float)midPoint - hysteresisVel) shouldSwap = true;
                }

                if (shouldSwap) {
                    juce::Component::SafePointer<SampleRegion> safeRegion(region);
                    performSwap(mIndex, neighborIndex, safeRegion.getComponent(), mouseScreenY);
                    swapped = true;
                    
                    srcCentre = (src.velocityLow + src.velocityHigh) / 2;
                    deltaVelCenter = targetCentre - srcCentre; 
                }
            }
        }
      };
    }
  }
}

void MainComponent::timerCallback() {
  const juce::ScopedLock sl(engine->getLock());
  float latestTime = -1.0f;
  uint32_t latestTrigger = 0;

  if (activeMappingIndex >= 0 &&
      activeMappingIndex < libraryData.mappings.size()) {
    auto &m = libraryData.mappings.getReference(activeMappingIndex);

    auto& synth = engine->getSynth();
    for (int i = 0; i < synth.getNumVoices(); ++i) {
      if (auto *v = dynamic_cast<SoteroSamplerVoice *>(synth.getVoice(i))) {
        // We only care about voices that are currently active (not Idle)
        // AND for our requested behavior: we stop the playhead if the user
        // released it (i.e., not in Attack/Decay/Sustain)
        if (v->isVoiceActive()) {
          auto *sound = dynamic_cast<const SoteroSamplerSound *>(
              v->getCurrentlyPlayingSound().get());

          if (sound && sound->getMidiRootNote() == m.midiNote &&
              sound->micLayer == m.micLayer) {

            // Find the most recently triggered voice (highest trigger time)
            uint32_t triggerTime = v->getTriggerTime();
            if (triggerTime >= latestTrigger) {
              latestTrigger = triggerTime;

              // To follow "return to start when stop clicking":
              // We check if the note is still being HELD.
              // JUCE uses 'isKeyDown' internally or we can check the ADSR
              // progress. If it's in Release, we'll treat it as -1.0 to reset.
              float progress = v->getADSRProgress();

              // If progress is greater than the start of Release, reset it.
              // Based on CurvedADSR::getVisualTimeElapsed, Release starts after
              // A + D + visualSustain
              float releaseStart = sound->adsrParams.attack +
                                   sound->adsrParams.decay +
                                   sound->adsrParams.visualSustain;

              if (progress >= 0.0f && progress < releaseStart) {
                latestTime = progress;
              } else {
                latestTime = -1.0f; // Reset to start
              }
            }
          }
        }
      }
    }
  }

  sculptingPanel.getADSR().getVisualizer().setPlayheadTime(latestTime);
}

void MainComponent::updateMetadataFromUI() {
  libraryData.name = metadataPanel.getNameEditor().getText();
  libraryData.author = metadataPanel.getAuthorEditor().getText();
}

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff121212));
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRect(getLocalBounds().reduced(5), 1.0f);
}

void MainComponent::deselectAllRegions() {
  activeMappingIndex = -1;
  sculptingPanel.setEnabled(false);
  sculptingPanel.getADSR().getVisualizer().setPlayheadTime(-1.0f);

  // Reset ADSR visualizer and sliders to default
  sculptingPanel.getADSR().setParams(0.1f, 0.1f, 1.0f, 0.2f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.5f);
  sculptingPanel.getFilter().setParams(0, 20000.0f, 0.1f);

  if (mappingPanel.layer1) {
    for (auto *col : mappingPanel.layer1->columns) {
      for (auto *region : col->regions)
        region->setActive(false);
    }
  }
  if (mappingPanel.layer2) {
    for (auto *col : mappingPanel.layer2->columns) {
      for (auto *region : col->regions)
        region->setActive(false);
    }
  }
}

void MainComponent::updateOctave(int newOctave) {
  mappingPanel.updateOctave(newOctave);
  updateGridUI();
  if (mappingPanel.layer1 && mappingPanel.layer1->keyboard)
    mappingPanel.layer1->keyboard->repaint();
  if (mappingPanel.layer2 && mappingPanel.layer2->keyboard)
    mappingPanel.layer2->keyboard->repaint();
}

void MainComponent::loadSoteroLibrary(const juce::File &file) {
  if (!file.existsAsFile())
    return;

  deselectAllRegions();
  engine->loadSoteroLibrary(file);

  metadataPanel.getNameEditor().setText(libraryData.name);
  metadataPanel.getAuthorEditor().setText(libraryData.author);

  updateGridUI();
}


void MainComponent::resized() {
  auto r = getLocalBounds();
  headerPanel.setBounds(r.removeFromTop(70));

  const float mappingWidthRatio = 0.65f;
  auto topArea = r.removeFromTop(350); // Increased from 220
  auto bottomArea = r;

  // 1. Bottom Area Setup (Mapping + Workspace)
  auto mappingBounds =
      bottomArea.removeFromLeft(bottomArea.getWidth() * mappingWidthRatio);
  mappingPanel.setBounds(mappingBounds.reduced(2));

  auto workspaceBounds = bottomArea;
  auto advancedArea =
      workspaceBounds.removeFromTop(workspaceBounds.getHeight() * 0.45f);
  advancedPanel.setBounds(advancedArea.reduced(2));
  metadataPanel.setBounds(workspaceBounds.reduced(2));

  // 2. Top Area Setup (Align with Bottom)
  // ADSR aligns with Workspace Area (Advanced + Metadata)
  sculptingPanel.setBounds(topArea.withLeft(workspaceBounds.getX())
                               .withWidth(workspaceBounds.getWidth())
                               .reduced(2));

  // Waveforms align with Layers (Layer 1/2) in Mapping Panel
  // We mirror the internal logic of MappingPanel (reduced(5) and gap of 24)
  auto innerMapping = mappingBounds.reduced(2).reduced(5);
  int gap = 24;
  int layerW = (innerMapping.getWidth() - gap) / 2;

  if (waveform1) {
    auto w1Bounds = topArea.withLeft(innerMapping.getX()).withWidth(layerW);
    waveform1->setBounds(w1Bounds.reduced(2));
  }

  if (waveform2) {
    auto w2Bounds =
        topArea.withLeft(innerMapping.getX() + layerW + gap).withWidth(layerW);
    waveform2->setBounds(w2Bounds.reduced(2));
  }
}



void MainComponent::alignLayers() {
  for (int note = 0; note < 128; ++note) {
    juce::Array<int> l1Indices, l2Indices;
    for (int i = 0; i < libraryData.mappings.size(); ++i) {
      auto &m = libraryData.mappings.getReference(i);
      if (m.samplePath.isEmpty() || m.midiNote != note)
        continue;
      if (m.micLayer == 0)
        l1Indices.add(i);
      else if (m.micLayer == 1)
        l2Indices.add(i);
    }

    // Sort by velocityLow
    auto sortByVel = [this](int a, int b) {
      return libraryData.mappings.getReference(a).velocityLow <
             libraryData.mappings.getReference(b).velocityLow;
    };
    std::sort(l1Indices.begin(), l1Indices.end(), sortByVel);
    std::sort(l2Indices.begin(), l2Indices.end(), sortByVel);

    // Link 1-to-1 by rank
    // Link 1-to-1 by rank
    int count = juce::jmin(l1Indices.size(), l2Indices.size());
    for (int k = 0; k < count; ++k) {
      auto &m1 = libraryData.mappings.getReference(l1Indices[k]);
      auto &m2 = libraryData.mappings.getReference(l2Indices[k]);

      m2.velocityLow = m1.velocityLow;
      m2.velocityHigh = m1.velocityHigh;
    }
  }
  updateGridUI();
  rebuildSynth();
}

int MainComponent::findCounterpart(int mIndex) {
  if (mIndex < 0 || mIndex >= libraryData.mappings.size())
    return -1;
  auto &src = libraryData.mappings.getReference(mIndex);

  juce::Array<int> srcIndices, dstIndices;
  for (int i = 0; i < libraryData.mappings.size(); ++i) {
    auto &m = libraryData.mappings.getReference(i);
    if (m.samplePath.isEmpty() || m.midiNote != src.midiNote)
      continue;
    if (m.micLayer == src.micLayer)
      srcIndices.add(i);
    else
      dstIndices.add(i);
  }

  // Sort both by velocity
  auto sortByVel = [this](int a, int b) {
    return libraryData.mappings.getReference(a).velocityLow <
           libraryData.mappings.getReference(b).velocityLow;
  };
  std::sort(srcIndices.begin(), srcIndices.end(), sortByVel);
  std::sort(dstIndices.begin(), dstIndices.end(), sortByVel);

  int rank = srcIndices.indexOf(mIndex);
  if (rank >= 0 && rank < dstIndices.size()) {
    return dstIndices[rank];
  }
  return -1;
}

// Calculates the pixel bounds that a region would occupy in its column
// given a specific velocity range, mirroring KeyColumn::resized() logic.
static juce::Rectangle<int> calcRegionBounds(const KeyColumn* col,
                                              int velLow, int velHigh) noexcept {
  float h      = (float)col->getHeight();
  float yTop   = h * (1.0f - (velHigh / 127.0f));
  float yBot   = h * (1.0f - (velLow  / 127.0f));
  return juce::Rectangle<int>(0, (int)yTop, col->getWidth(),
                               (int)juce::jmax(1.0f, yBot - yTop));
}

// In-place, atomic swap of velocity zones.
// NO updateGridUI() — components are kept alive so the drag continues.
// Uses juce::ComponentAnimator to give a smooth ghost-glide effect.
void MainComponent::performSwap(int mIndexA, int mIndexB,
                                SampleRegion* draggedRegion,
                                int mouseScreenY) {
  if (mIndexA < 0 || mIndexA >= libraryData.mappings.size()) return;
  if (mIndexB < 0 || mIndexB >= libraryData.mappings.size()) return;
  if (mIndexA == mIndexB) return;

  auto &mA = libraryData.mappings.getReference(mIndexA);
  auto &mB = libraryData.mappings.getReference(mIndexB);

  // Safety: both must be in the same column
  if (mA.midiNote != mB.midiNote || mA.micLayer != mB.micLayer) return;

  // Find the target region's UI component BEFORE the data swap
  SampleRegion* targetRegion = findRegionForIndex(mIndexB);

  // Helper: animate a region from its current bounds to new bounds
  // useProxyComponent=true creates a ghost screenshot that glides while
  // the real component is immediately at its final position.
  auto animateTo = [&](SampleRegion* r, int velLow, int velHigh) {
    if (r == nullptr) return;
    if (auto* col = dynamic_cast<KeyColumn*>(r->getParentComponent())) {
      auto newBounds = calcRegionBounds(col, velLow, velHigh);
      juce::Desktop::getInstance().getAnimator().animateComponent(
          r, newBounds, 1.0f, 250 /*ms*/, false /*lerp real component*/, 0.0, 0.0);
    }
  };

  // === ATOMIC DATA SWAP ===
  int tmpLo = mA.velocityLow;  int tmpHi = mA.velocityHigh;
  mA.velocityLow  = mB.velocityLow;  mA.velocityHigh = mB.velocityHigh;
  mB.velocityLow  = tmpLo;           mB.velocityHigh = tmpHi;

  // === ANIMATE TARGET (its velocity zone moved) ===
  animateTo(targetRegion, mB.velocityLow, mB.velocityHigh);
  if (targetRegion) targetRegion->updateFromMapping(mB);

  // === UPDATE & RESET DRAGGED REGION ===
  if (draggedRegion) {
    draggedRegion->updateFromMapping(mA);
    // Animate dragged region to its new (swapped) velocity position as well
    animateTo(draggedRegion, mA.velocityLow, mA.velocityHigh);
    // Reset drag anchors so the next swap threshold is evaluated from
    // the new position — this is what enables continuous multi-swap.
    draggedRegion->resetSwapState(mouseScreenY);
  }

  // === SYNC LAYERS ===
  if (mappingPanel.layerSyncLock.getToggleState()) {
    int mirrorA = findCounterpart(mIndexA);
    int mirrorB = findCounterpart(mIndexB);
    if (mirrorA != -1 && mirrorB != -1 && mirrorA != mirrorB) {
      SampleRegion* mirrorTargetRegion = findRegionForIndex(mirrorB);
      auto &mMA = libraryData.mappings.getReference(mirrorA);
      auto &mMB = libraryData.mappings.getReference(mirrorB);
      int mTmpLo = mMA.velocityLow; int mTmpHi = mMA.velocityHigh;
      mMA.velocityLow  = mMB.velocityLow;  mMA.velocityHigh = mMB.velocityHigh;
      mMB.velocityLow  = mTmpLo;           mMB.velocityHigh = mTmpHi;
      animateTo(mirrorTargetRegion, mMB.velocityLow, mMB.velocityHigh);
      if (mirrorTargetRegion) mirrorTargetRegion->updateFromMapping(mMB);
      SampleRegion* mirrorDragged = findRegionForIndex(mirrorA);
      if (mirrorDragged) mirrorDragged->updateFromMapping(mMA);
    }
  }

  // Rebuild audio asynchronously (non-blocking: won't destroy UI components)
  juce::MessageManager::callAsync([this] { rebuildSynth(); });
}


// Finds the SampleRegion UI component corresponding to a mapping index.
// Matches by counting how many non-empty mappings with the same note+layer
// appear before mappingIndex in libraryData.mappings — that rank is the
// position of the region in KeyColumn::regions.
void MainComponent::updateColumnRegions(int note, int layer) {
  int baseNote = (mappingPanel.currentOctave + 3) * 12;
  int colIndex = note - baseNote;
  if (colIndex < 0 || colIndex >= 12) return;

  auto* layerView = (layer == 0) ? mappingPanel.layer1.get()
                                 : mappingPanel.layer2.get();
  if (layerView && layerView->columns[colIndex]) {
    auto* col = layerView->columns[colIndex];
    // Sync UI components with data before recalculating bounds
    for (int i = 0; i < libraryData.mappings.size(); ++i) {
      auto& m = libraryData.mappings.getReference(i);
      if (m.samplePath.isNotEmpty() && m.midiNote == note && m.micLayer == layer) {
        if (auto* reg = findRegionForIndex(i))
          reg->updateFromMapping(m);
      }
    }
    col->resized();
  }
}

SampleRegion* MainComponent::findRegionForIndex(int mappingIndex) {
  if (mappingIndex < 0 || mappingIndex >= libraryData.mappings.size())
    return nullptr;

  const auto &m = libraryData.mappings.getReference(mappingIndex);
  if (m.samplePath.isEmpty()) return nullptr;

  int baseNote = (mappingPanel.currentOctave + 3) * 12;
  int colIndex = m.midiNote - baseNote;
  if (colIndex < 0 || colIndex >= 12) return nullptr;

  auto* targetLayer = (m.micLayer == 0) ? mappingPanel.layer1.get()
                                        : mappingPanel.layer2.get();
  if (!targetLayer) return nullptr;

  KeyColumn* col = targetLayer->columns[colIndex];
  if (!col) return nullptr;

  // Persistent ID lookup: iterate through regions in this column
  // and find the one that owns this specific mapping index.
  for (auto* reg : col->regions) {
      if (reg->getMappingIndex() == mappingIndex)
          return reg;
  }
  return nullptr;
}

bool MainComponent::isRangeFree(int note, int micLayer, int lo, int hi,
                                int excludeIndex) {
  for (int i = 0; i < libraryData.mappings.size(); ++i) {
    if (i == excludeIndex)
      continue;
    const auto &m = libraryData.mappings.getReference(i);
    if (m.samplePath.isEmpty())
      continue;
    if (m.midiNote == note && m.micLayer == micLayer) {
      if (!(hi < m.velocityLow || lo > m.velocityHigh))
        return false;
    }
  }
  return true;
}

void MainComponent::applyDefinitiveCollision(int targetIndex, const KeyMapping &proposed, int modeVal, bool isSwapPhase) {
  if (targetIndex < 0 || targetIndex >= libraryData.mappings.size()) return;

  auto dragMode = (sotero::SampleRegion::DragMode)modeVal;
  auto &target = libraryData.mappings.getReference(targetIndex);
  int note = target.midiNote;
  int layer = target.micLayer;

  // 1. Get all siblings in this column
  juce::Array<int> colIndices;
  for (int i = 0; i < libraryData.mappings.size(); ++i) {
    auto &m = libraryData.mappings.getReference(i);
    if (m.samplePath.isNotEmpty() && m.midiNote == note && m.micLayer == layer) {
      colIndices.add(i);
    }
  }

  // 2. Sort by current structural position
  std::sort(colIndices.begin(), colIndices.end(), [&](int a, int b) {
    return libraryData.mappings.getReference(a).velocityLow < libraryData.mappings.getReference(b).velocityLow;
  });

  int rank = colIndices.indexOf(targetIndex);
  if (rank == -1) return;

  target.velocityLow = proposed.velocityLow;
  target.velocityHigh = proposed.velocityHigh;

  if (isSwapPhase && dragMode == sotero::SampleRegion::DragMode::Body) {
    // SWAP MODE (Alt+Drag) -> Reorder and Pack
    std::sort(colIndices.begin(), colIndices.end(), [&](int a, int b) {
      float cA = (float)(libraryData.mappings.getReference(a).velocityLow + libraryData.mappings.getReference(a).velocityHigh) / 2.0f;
      float cB = (float)(libraryData.mappings.getReference(b).velocityLow + libraryData.mappings.getReference(b).velocityHigh) / 2.0f;
      if (std::abs(cA - cB) < 0.001f) return a == targetIndex; // Target wins tie-breakers
      return cA < cB;
    });

    int newRank = colIndices.indexOf(targetIndex);

    // Pack downwards
    for (int i = newRank - 1; i >= 0; --i) {
      auto &curr = libraryData.mappings.getReference(colIndices[i]);
      auto &above = libraryData.mappings.getReference(colIndices[i + 1]);
      if (curr.velocityHigh >= above.velocityLow) {
        int r = std::max(1, curr.velocityHigh - curr.velocityLow);
        curr.velocityHigh = above.velocityLow - 1;
        curr.velocityLow = curr.velocityHigh - r;
      }
      if (curr.velocityLow < 0) {
        curr.velocityLow = 0;
        curr.velocityHigh = std::max(1, curr.velocityHigh);
      }
    }

    // Pack upwards
    for (int i = newRank + 1; i < colIndices.size(); ++i) {
      auto &curr = libraryData.mappings.getReference(colIndices[i]);
      auto &below = libraryData.mappings.getReference(colIndices[i - 1]);
      if (curr.velocityLow <= below.velocityHigh) {
        int r = std::max(1, curr.velocityHigh - curr.velocityLow);
        curr.velocityLow = below.velocityHigh + 1;
        curr.velocityHigh = curr.velocityLow + r;
      }
      if (curr.velocityHigh > 127) {
        curr.velocityHigh = 127;
        curr.velocityLow = std::min(126, curr.velocityLow);
      }
    }

    // Final boundary check
    for (int i = 0; i < colIndices.size(); ++i) {
        auto &m = libraryData.mappings.getReference(colIndices[i]);
        if (m.velocityLow < 0) m.velocityLow = 0;
        if (m.velocityHigh > 127) m.velocityHigh = 127;
        if (m.velocityLow > m.velocityHigh) {
            m.velocityLow = std::max(0, m.velocityHigh - 1);
        }
    }
  } else {
    // NORMAL DRAG MODE -> Resolve collisions recursively
    int targetLo = proposed.velocityLow;
    int targetHi = proposed.velocityHigh;
    resolveCollisions(note, layer, targetLo, targetHi, targetIndex, true, true);
    target.velocityLow = targetLo;
    target.velocityHigh = targetHi;
  }
}


void MainComponent::resolveCollisions(int note, int micLayer, int &targetLo,
                                      int &targetHi, int excludeIndex,
                                      bool allowCrossSync,
                                      bool isPrimaryTarget) {
  static int depth = 0;
  if (++depth > 128) {
    --depth;
    return;
  }

  bool syncActive =
      allowCrossSync && mappingPanel.layerSyncLock.getToggleState();

  // 1. Identify and categorize neighbors
  struct Neighbor {
    int index;
    int lo, hi;
    bool operator==(const Neighbor &other) const {
      return index == other.index;
    }
  };
  juce::Array<Neighbor> above, below;

  for (int i = 0; i < libraryData.mappings.size(); ++i) {
    if (i == excludeIndex)
      continue;
    auto &m = libraryData.mappings.getReference(i);
    if (m.samplePath.isEmpty() || m.midiNote != note || m.micLayer != micLayer)
      continue;

    // Strict positional categorization
    if (m.velocityLow >= targetHi ||
        (m.velocityLow > targetLo && m.velocityHigh > targetHi))
      above.add({i, m.velocityLow, m.velocityHigh});
    else
      below.add({i, m.velocityLow, m.velocityHigh});
  }

  // 2. Sort outward from target
  std::sort(above.begin(), above.end(),
            [](const Neighbor &a, const Neighbor &b) { return a.lo < b.lo; });
  std::sort(below.begin(), below.end(),
            [](const Neighbor &a, const Neighbor &b) { return a.hi > b.hi; });

  auto syncNeighbor = [this, note, micLayer, syncActive](int idx) {
    if (syncActive) {
      int otherIdx = findCounterpart(idx);
      if (otherIdx != -1) {
        auto &orig = libraryData.mappings.getReference(idx);
        auto &other = libraryData.mappings.getReference(otherIdx);
        other.velocityLow = orig.velocityLow;
        other.velocityHigh = orig.velocityHigh;
        resolveCollisions(note, 1 - micLayer, other.velocityLow,
                          other.velocityHigh, otherIdx, true, false);
      }
    }
  };

  // 3. Resolve Above (Stick/Glue then Push)
  for (auto &c : above) {
    auto &m = libraryData.mappings.getReference(c.index);

    // ADHESIVE (PULL): If we were glued at start
    bool shouldPull = isPrimaryTarget && dragStickyTop && (above.indexOf(c) == 0);

    if (targetHi >= m.velocityLow || shouldPull) {

      int oldLo = m.velocityLow;
      int oldHi = m.velocityHigh;

      // GLUE LOGIC: Move the neighbor's bottom edge (resize it)
      int newMLo = targetHi + 1;
      int newMHi = oldHi; // Default: Keep top edge (Resize)

      // If neighbor would be squashed, it MUST push
      if (newMHi <= newMLo) {
        int mRange = juce::jmax(1, oldHi - oldLo);
        newMHi = juce::jlimit(0, 127, newMLo + mRange);
        if (newMHi == 127)
          newMLo = 127 - mRange;
      }

      resolveCollisions(note, micLayer, newMLo, newMHi, c.index, allowCrossSync,
                        false);
      m.velocityLow = newMLo;
      m.velocityHigh = newMHi;

      if (m.velocityLow <= targetHi) {
        targetHi = juce::jmax(0, m.velocityLow - 1);
        if (targetLo > targetHi)
          targetLo = targetHi;
      }

      if (m.velocityLow != oldLo || m.velocityHigh != oldHi)
        syncNeighbor(c.index);
    }
  }

  // 4. Resolve Below (Stick/Glue then Push)
  for (auto &c : below) {
    auto &m = libraryData.mappings.getReference(c.index);

    // ADHESIVE (PULL): If we were glued at start
    // bool shouldPull = isPrimaryTarget && dragStickyBottom && (below.indexOf(c) == 0);
    bool shouldPull = false; // Placeholder

    if (targetLo <= m.velocityHigh || shouldPull) {
      int oldLo = m.velocityLow;
      int oldHi = m.velocityHigh;

      // GLUE LOGIC: Move the neighbor's top edge (resize it)
      int newMHi = targetLo - 1;
      int newMLo = oldLo; // Default: Keep bottom edge (Resize)

      // If neighbor would be squashed, it MUST push
      if (newMLo >= newMHi) {
        int mRange = juce::jmax(1, oldHi - oldLo);
        newMLo = juce::jlimit(0, 127, newMHi - mRange);
        if (newMLo == 0)
          newMHi = mRange;
      }

      resolveCollisions(note, micLayer, newMLo, newMHi, c.index, allowCrossSync,
                        false);
      m.velocityLow = newMLo;
      m.velocityHigh = newMHi;

      if (m.velocityHigh >= targetLo) {
        targetLo = juce::jmin(127, m.velocityHigh + 1);
        if (targetHi < targetLo)
          targetHi = targetLo;
      }

      if (m.velocityLow != oldLo || m.velocityHigh != oldHi)
        syncNeighbor(c.index);
    }
  }

  --depth;
}


void MainComponent::filesDropped(const juce::StringArray &files, int x, int y) {
  // Global drops are now handled by individual KeyColumns.
  // We keep this empty or for non-target drops if needed.
}

} // namespace sotero
