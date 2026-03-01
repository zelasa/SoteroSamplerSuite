#include "PluginEditor.h"
#include "PluginProcessor.h"

// --- PerformanceView Implementation ---

PerformanceView::PerformanceView(SamplerPlayerAudioProcessor &p)
    : processor(p), threshKnob("THRESH"), ratioKnob("RATIO"),
      attackKnob("ATTACK"), releaseKnob("RELEASE"), revSizeKnob("SIZE"),
      revMixKnob("MIX"), masterVolKnob("MASTER") {
  // Velocity Curve
  addAndMakeVisible(velocityGroup);
  velocityGroup.setText("VELOCITY CURVE");
  const juce::String curveNames[] = {"SOFT", "LINEAR", "HARD"};
  for (int i = 0; i < 3; ++i) {
    curveButtons[i] = std::make_unique<CurveButton>(curveNames[i], i);
    addAndMakeVisible(*curveButtons[i]);
    curveButtons[i]->onClick = [this, i] {
      auto *param = processor.apvts.getParameter("velocityCurve");
      if (param)
        param->setValueNotifyingHost(i / 2.0f);
    };
  }

  // Dynamics (Compressor)
  addAndMakeVisible(dynamicsGroup);
  dynamicsGroup.setText("MASTER COMPRESSOR");
  const juce::String compNames[] = {"OFF", "LIGHT", "MID", "HARD"};
  for (int i = 0; i < 4; ++i) {
    compModes[i] = std::make_unique<ModeButton>(compNames[i], 2002);
    addAndMakeVisible(*compModes[i]);
    compModes[i]->onClick = [this, i] {
      auto *param = processor.apvts.getParameter("masterComp");
      if (param)
        param->setValueNotifyingHost(i / 3.0f);
    };
  }

  addAndMakeVisible(threshKnob);
  threshAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.apvts, "compThresh", threshKnob.getSlider());

  addAndMakeVisible(ratioKnob);
  ratioAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.apvts, "compRatio", ratioKnob.getSlider());

  addAndMakeVisible(attackKnob);
  attackAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.apvts, "compAttack", attackKnob.getSlider());

  addAndMakeVisible(releaseKnob);
  releaseAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.apvts, "compRelease", releaseKnob.getSlider());

  // Spatial (Reverb)
  addAndMakeVisible(spatialGroup);
  spatialGroup.setText("REVERB");

  addAndMakeVisible(revToggle);
  revToggle.setButtonText("ENABLE");
  revEnableAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processor.apvts, "revEnable", revToggle);

  const juce::String revNames[] = {"ROOM", "HALL", "PLATE", "STAD"};
  for (int i = 0; i < 4; ++i) {
    revModes[i] = std::make_unique<ModeButton>(revNames[i], 2003);
    addAndMakeVisible(*revModes[i]);
    revModes[i]->onClick = [this, i] {
      auto *param = processor.apvts.getParameter("revType");
      if (param)
        param->setValueNotifyingHost(i / 3.0f);
    };
  }

  addAndMakeVisible(revSizeKnob);
  revSizeAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.apvts, "revSize", revSizeKnob.getSlider());

  addAndMakeVisible(revMixKnob);
  revMixAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.apvts, "revMix", revMixKnob.getSlider());

  // Master Area
  addAndMakeVisible(masterGroup);
  masterGroup.setText("OUTPUT");

  addAndMakeVisible(masterVolKnob);
  masterVolAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.apvts, "masterVol", masterVolKnob.getSlider());

  addAndMakeVisible(masterVU_L);
  addAndMakeVisible(masterVU_R);

  startTimerHz(30);
}

void PerformanceView::paint(juce::Graphics &g) {}

void PerformanceView::timerCallback() {
  // Sync Radio buttons for Curve
  int curveType = (int)*processor.apvts.getRawParameterValue("velocityCurve");
  for (int i = 0; i < 3; ++i)
    curveButtons[i]->setToggleState(i == curveType, juce::dontSendNotification);

  // Sync Radio buttons for Compressor Mode
  int compType = (int)*processor.apvts.getRawParameterValue("masterComp");
  for (int i = 0; i < 4; ++i)
    compModes[i]->setToggleState(i == compType, juce::dontSendNotification);

  // Sync Radio buttons for Reverb Type
  int revType = (int)*processor.apvts.getRawParameterValue("revType");
  for (int i = 0; i < 4; ++i)
    revModes[i]->setToggleState(i == revType, juce::dontSendNotification);

  // Update Master VU
  masterVU_L.setLevel(processor.getMasterLevelL());
  masterVU_R.setLevel(processor.getMasterLevelR());
}

void PerformanceView::resized() {
  auto area = getLocalBounds().reduced(20);

  auto leftCol = area.removeFromLeft(area.getWidth() * 0.48f);
  area.removeFromLeft(30); // Spacer
  auto rightCol = area;

  // --- LEFT COLUMN (Velocity & Dynamics) ---
  // Large Velocity Curve area
  auto velArea = leftCol.removeFromTop(200);
  velocityGroup.setBounds(velArea.reduced(2));
  auto velInner = velArea.reduced(25, 45);
  int unitW = velInner.getWidth() / 3;
  for (int i = 0; i < 3; ++i)
    curveButtons[i]->setBounds(velInner.removeFromLeft(unitW).reduced(10));

  // Massive Dynamics Section
  dynamicsGroup.setBounds(leftCol.reduced(2));
  auto dynInner = dynamicsGroup.getBounds().reduced(20, 30);

  auto modesRow = dynInner.removeFromTop(60);
  int modeW = modesRow.getWidth() / 4;
  for (int i = 0; i < 4; ++i)
    compModes[i]->setBounds(modesRow.removeFromLeft(modeW).reduced(4));

  auto knobsArea = dynInner.reduced(10);
  auto kRow1 = knobsArea.removeFromTop(knobsArea.getHeight() / 2);
  int kw = kRow1.getWidth() / 2;
  // Large Compressor Knobs
  threshKnob.setBounds(kRow1.removeFromLeft(kw).reduced(8));
  ratioKnob.setBounds(kRow1.reduced(8));
  attackKnob.setBounds(knobsArea.removeFromLeft(kw).reduced(8));
  releaseKnob.setBounds(knobsArea.reduced(8));

  // --- RIGHT COLUMN (Spatial & Master) ---
  // BOLD REVERB SECTION
  auto spatArea = rightCol.removeFromTop(320);
  spatialGroup.setBounds(spatArea.reduced(2));
  auto spatInner = spatArea.reduced(25, 40);

  // Big Enable Toggle
  revToggle.setBounds(spatInner.removeFromTop(50));

  // Big Mode Buttons
  auto rModesRow = spatInner.removeFromTop(60);
  int rModeW = rModesRow.getWidth() / 4;
  for (int i = 0; i < 4; ++i)
    revModes[i]->setBounds(rModesRow.removeFromLeft(rModeW).reduced(4));

  // MASSIVE REVERB KNOBS
  auto rKnobsArea = spatInner.reduced(10);
  int rkw = rKnobsArea.getWidth() / 2;
  revSizeKnob.setBounds(rKnobsArea.removeFromLeft(rkw).reduced(5));
  revMixKnob.setBounds(rKnobsArea.reduced(5));

  // HERO MASTER SECTION
  masterGroup.setBounds(rightCol.reduced(2));
  auto mastInner = masterGroup.getBounds().reduced(20, 30);

  // Stereo VU on the right
  auto vuArea = mastInner.removeFromRight(100);
  masterVU_L.setBounds(vuArea.removeFromLeft(45).reduced(5, 10));
  masterVU_R.setBounds(vuArea.reduced(5, 10));

  // Hero Master Volume
  masterVolKnob.setBounds(mastInner.reduced(10, 5));
}

// --- SetupView Implementation ---

SetupView::SetupView(SamplerPlayerAudioProcessor &p) : processor(p) {
  addAndMakeVisible(globalGroup);
  globalGroup.setText("GLOBAL MIDI SETTINGS");

  addAndMakeVisible(chanLabel);
  chanLabel.setText("Channel:", juce::dontSendNotification);
  addAndMakeVisible(chanCombo);
  chanCombo.addItem("All Channels", 1);
  for (int i = 1; i <= 16; ++i)
    chanCombo.addItem("Channel " + juce::String(i), i + 1);
  chanAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          p.apvts, "midiChannel", chanCombo);

  addAndMakeVisible(noteLabel);
  noteLabel.setText("Trigger Note:", juce::dontSendNotification);
  addAndMakeVisible(noteCombo);
  for (int i = 0; i <= 127; ++i)
    noteCombo.addItem(juce::MidiMessage::getMidiNoteName(i, true, true, 3) +
                          " (" + juce::String(i) + ")",
                      i + 1);
  noteAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          p.apvts, "midiNote", noteCombo);

  const juce::String names[] = {"LOW (Note < 55)", "MID (Note 55-104)",
                                "HIGH (Note > 104)"};
  for (int i = 0; i < 3; ++i) {
    trackControls[i] = std::make_unique<TrackControl>(p, i, names[i]);
    addAndMakeVisible(*trackControls[i]);
  }
}

void SetupView::resized() {
  auto area = getLocalBounds().reduced(30);

  auto gArea = area.removeFromTop(100);
  globalGroup.setBounds(gArea);
  auto gInner = gArea.reduced(30, 20);

  int unitW = gInner.getWidth() / 2;
  auto chanArea = gInner.removeFromLeft(unitW);
  chanLabel.setFont(juce::Font(16.0f, juce::Font::bold));
  chanLabel.setBounds(chanArea.removeFromLeft(100));
  chanCombo.setBounds(chanArea.reduced(0, 10));

  noteLabel.setFont(juce::Font(16.0f, juce::Font::bold));
  noteLabel.setBounds(gInner.removeFromLeft(125));
  noteCombo.setBounds(gInner.reduced(0, 10));

  area.removeFromTop(30); // huge spacer
  int tW = area.getWidth() / 3;
  for (int i = 0; i < 3; ++i)
    trackControls[i]->setBounds(area.removeFromLeft(tW).reduced(12));
}

// --- TrackControl Implementation ---

TrackControl::TrackControl(SamplerPlayerAudioProcessor &p, int index,
                           const juce::String &name)
    : processor(p), trackIndex(index), volSlider("VOL"), panSlider("PAN") {
  addAndMakeVisible(titleLabel);
  titleLabel.setText(name, juce::dontSendNotification);
  titleLabel.setJustificationType(juce::Justification::centred);
  titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));

  addAndMakeVisible(nameLabel);
  nameLabel.setText("NO SAMPLE", juce::dontSendNotification);
  nameLabel.setJustificationType(juce::Justification::centred);
  nameLabel.setColour(juce::Label::textColourId, juce::Colours::orange);

  addAndMakeVisible(volSlider);
  volAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.apvts, "vol" + juce::String(index), volSlider.getSlider());

  addAndMakeVisible(panSlider);
  panAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.apvts, "pan" + juce::String(index), panSlider.getSlider());

  addAndMakeVisible(loadBtn);
  loadBtn.setButtonText("LOAD");
  loadBtn.onClick = [this] {
    auto chooser = std::make_shared<juce::FileChooser>("Select", juce::File{},
                                                       "*.wav;*.aif;*.mp3");
    chooser->launchAsync(juce::FileBrowserComponent::openMode,
                         [this, chooser](const juce::FileChooser &fc) {
                           auto file = fc.getResult();
                           if (file.existsAsFile())
                             processor.loadTrackSample(trackIndex, file);
                         });
  };

  addAndMakeVisible(vu);
  startTimerHz(30);
}

void TrackControl::paint(juce::Graphics &g) {
  g.setColour(juce::Colour(0xFF1E1E1E));
  g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2), 10.0f);
  g.setColour(juce::Colours::white.withAlpha(0.15f));
  g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 10.0f, 1.5f);
}

void TrackControl::resized() {
  auto area = getLocalBounds().reduced(15);

  // 1. Header (Bold Title + Professional Load)
  auto header = area.removeFromTop(60);
  titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
  loadBtn.setBounds(header.removeFromRight(100).reduced(5));
  titleLabel.setBounds(header);

  // 2. Info area - Large Bold Text
  area.removeFromTop(10);
  nameLabel.setFont(juce::Font(18.0f, juce::Font::bold));
  nameLabel.setBounds(area.removeFromTop(35));
  area.removeFromTop(10);

  // 3. Bottom controls (Volume and Pan)
  auto bottom = area.removeFromBottom(150);
  auto knobsRow = bottom.removeFromTop(120);
  int kw = knobsRow.getWidth() / 2;
  volSlider.setBounds(knobsRow.removeFromLeft(kw).reduced(5, 0));
  panSlider.setBounds(knobsRow.reduced(5, 0));

  // 4. Massive VU Meter in the middle
  area.removeFromBottom(10); // spacer
  vu.setBounds(area.reduced(2, 0));
}

void TrackControl::timerCallback() {
  nameLabel.setText(processor.getTrackSampleName(trackIndex).toUpperCase(),
                    juce::dontSendNotification);
  vu.setLevel(processor.getTrackLevel(trackIndex));
}

// --- Editor Implementation ---

SamplerPlayerAudioProcessorEditor::SamplerPlayerAudioProcessorEditor(
    SamplerPlayerAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      keyboardComponent(p.getKeyboardState(),
                        juce::MidiKeyboardComponent::horizontalKeyboard) {
  addAndMakeVisible(performBtn);
  performBtn.setButtonText("PERFORMANCE");
  performBtn.setClickingTogglesState(true);
  performBtn.setRadioGroupId(100);
  performBtn.setToggleState(true, juce::dontSendNotification);
  performBtn.onClick = [this] {
    performanceView->setVisible(true);
    setupView->setVisible(false);
  };

  addAndMakeVisible(setupBtn);
  setupBtn.setButtonText("SETUP");
  setupBtn.setClickingTogglesState(true);
  setupBtn.setRadioGroupId(100);
  setupBtn.onClick = [this] {
    performanceView->setVisible(false);
    setupView->setVisible(true);
  };

  addAndMakeVisible(midiMonitorLabel);
  midiMonitorLabel.setText("MIDI IN: ---", juce::dontSendNotification);
  midiMonitorLabel.setJustificationType(juce::Justification::centredRight);
  midiMonitorLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
  midiMonitorLabel.setFont(juce::Font(16.0f, juce::Font::bold));

  addAndMakeVisible(midiVelocityLabel);
  midiVelocityLabel.setText("VELOCITY: ---", juce::dontSendNotification);
  midiVelocityLabel.setJustificationType(juce::Justification::centredRight);
  midiVelocityLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
  midiVelocityLabel.setFont(juce::Font(16.0f, juce::Font::bold));

  addAndMakeVisible(logo);
  auto logoImg = juce::ImageFileFormat::loadFrom(BinaryData::logo_png,
                                                 BinaryData::logo_pngSize);
  logo.setImage(logoImg, juce::RectanglePlacement::xLeft |
                             juce::RectanglePlacement::yMid);

  performanceView = std::make_unique<PerformanceView>(audioProcessor);
  addAndMakeVisible(*performanceView);

  setupView = std::make_unique<SetupView>(audioProcessor);
  addAndMakeVisible(*setupView);
  setupView->setVisible(false);

  addAndMakeVisible(keyboardComponent);
  keyboardComponent.setAvailableRange(21, 108); // Full 88-key piano
  keyboardComponent.setKeyWidth(21.0f);         // Fits 52 white keys in 1100px

  setSize(1100, 850);
  startTimerHz(30);
}

SamplerPlayerAudioProcessorEditor::~SamplerPlayerAudioProcessorEditor() {}

void SamplerPlayerAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xFF121212));
}

void SamplerPlayerAudioProcessorEditor::resized() {
  auto area = getLocalBounds();
  auto topBar = area.removeFromTop(70).reduced(15, 10); // Dashboard Header

  // 1. Logo (Left)
  logo.setBounds(topBar.removeFromLeft(150));

  // 2. MIDI Monitor (Right - Bold & Professional)
  auto monitorArea = topBar.removeFromRight(200);
  midiMonitorLabel.setBounds(
      monitorArea.removeFromTop(monitorArea.getHeight() / 2));
  midiVelocityLabel.setBounds(monitorArea);

  // 3. Navigation Tabs (Center)
  auto tabs = topBar.reduced(50, 5);
  int tabW = tabs.getWidth() / 2;
  performBtn.setBounds(tabs.removeFromLeft(tabW).reduced(15, 0));
  setupBtn.setBounds(tabs.reduced(15, 0));

  keyboardComponent.setBounds(area.removeFromBottom(100));

  performanceView->setBounds(area);
  setupView->setBounds(area);
}

void SamplerPlayerAudioProcessorEditor::timerCallback() {
  // Global MIDI Monitoring
  int lastNote = audioProcessor.getLastMidiNote();
  int lastVel = audioProcessor.getLastMidiVelocity();
  if (lastNote >= 0) {
    midiMonitorLabel.setText(
        "MIDI IN: " +
            juce::MidiMessage::getMidiNoteName(lastNote, true, true, 3) + " (" +
            juce::String(lastNote) + ")",
        juce::dontSendNotification);
    midiVelocityLabel.setText("VELOCITY: " + juce::String(lastVel),
                              juce::dontSendNotification);
  } else {
    midiMonitorLabel.setText("MIDI IN: ---", juce::dontSendNotification);
    midiVelocityLabel.setText("VELOCITY: ---", juce::dontSendNotification);
  }
}
