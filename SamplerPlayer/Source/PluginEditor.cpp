#include "PluginEditor.h"
#include "PluginProcessor.h"

// --- PerformanceView Implementation ---

PerformanceView::PerformanceView(sotero::ISoteroAudioEngine &e)
    : engine(e), threshKnob("THRESH"), ratioKnob("RATIO"), attackKnob("ATTACK"),
      releaseKnob("RELEASE"), revSizeKnob("SIZE"), revMixKnob("MIX"),
      masterVolKnob("MASTER") {
  // Velocity Curve
  addAndMakeVisible(velocityGroup);
  velocityGroup.setText("VELOCITY CURVE");
  const juce::String curveNames[] = {"SOFT", "LINEAR", "HARD"};
  for (int i = 0; i < 3; ++i) {
    curveButtons[i] = std::make_unique<sotero::CurveButton>(curveNames[i], i);
    addAndMakeVisible(*curveButtons[i]);
    curveButtons[i]->onClick = [this, i] {
      auto *param = engine.getAPVTS().getParameter("velocityCurve");
      if (param)
        param->setValueNotifyingHost(i / 2.0f);
    };
  }

  // Dynamics (Compressor)
  addAndMakeVisible(dynamicsGroup);
  dynamicsGroup.setText("MASTER COMPRESSOR");
  const juce::String compNames[] = {"OFF", "LIGHT", "MID", "HARD"};
  for (int i = 0; i < 4; ++i) {
    compModes[i] = std::make_unique<sotero::ModeButton>(compNames[i], 2002);
    addAndMakeVisible(*compModes[i]);
    compModes[i]->onClick = [this, i] {
      auto *param = engine.getAPVTS().getParameter("masterComp");
      if (param)
        param->setValueNotifyingHost(i / 3.0f);
    };
  }

  addAndMakeVisible(threshKnob);
  threshAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "compThresh", threshKnob.getSlider());

  addAndMakeVisible(ratioKnob);
  ratioAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "compRatio", ratioKnob.getSlider());

  addAndMakeVisible(attackKnob);
  attackAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "compAttack", attackKnob.getSlider());

  addAndMakeVisible(releaseKnob);
  releaseAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "compRelease", releaseKnob.getSlider());

  // Spatial (Reverb)
  addAndMakeVisible(spatialGroup);
  spatialGroup.setText("REVERB");

  addAndMakeVisible(revToggle);
  revToggle.setButtonText("ENABLE");
  revEnableAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          engine.getAPVTS(), "revEnable", revToggle);

  const juce::String revNames[] = {"ROOM", "HALL", "PLATE", "STAD"};
  for (int i = 0; i < 4; ++i) {
    revModes[i] = std::make_unique<sotero::ModeButton>(revNames[i], 2003);
    addAndMakeVisible(*revModes[i]);
    revModes[i]->onClick = [this, i] {
      auto *param = engine.getAPVTS().getParameter("revType");
      if (param)
        param->setValueNotifyingHost(i / 3.0f);
    };
  }

  addAndMakeVisible(revSizeKnob);
  revSizeAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "revSize", revSizeKnob.getSlider());

  addAndMakeVisible(revMixKnob);
  revMixAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "revMix", revMixKnob.getSlider());

  // Master Area
  addAndMakeVisible(masterGroup);
  masterGroup.setText("OUTPUT");

  addAndMakeVisible(masterVolKnob);
  masterVolAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "masterVol", masterVolKnob.getSlider());

  addAndMakeVisible(masterVU_L);
  addAndMakeVisible(masterVU_R);

  startTimerHz(30);
}

PerformanceView::~PerformanceView() { stopTimer(); }

void PerformanceView::paint(juce::Graphics &g) {}

void PerformanceView::timerCallback() {
  // Sync Radio buttons for Curve
  int curveType = (int)*engine.getAPVTS().getRawParameterValue("velocityCurve");
  for (int i = 0; i < 3; ++i)
    curveButtons[i]->setToggleState(i == curveType, juce::dontSendNotification);

  // Sync Radio buttons for Compressor Mode
  int compType = (int)*engine.getAPVTS().getRawParameterValue("masterComp");
  for (int i = 0; i < 4; ++i)
    compModes[i]->setToggleState(i == compType, juce::dontSendNotification);

  // Sync Radio buttons for Reverb Type
  int revType = (int)*engine.getAPVTS().getRawParameterValue("revType");
  for (int i = 0; i < 4; ++i)
    revModes[i]->setToggleState(i == revType, juce::dontSendNotification);

  // Update Master VU
  masterVU_L.setLevel(engine.getLevelL());
  masterVU_R.setLevel(engine.getLevelR());
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

SetupView::SetupView(sotero::ISoteroAudioEngine &e) : engine(e) {
  addAndMakeVisible(globalGroup);
  globalGroup.setText("LIBRARY SETUP");

  addAndMakeVisible(chanLabel);
  chanLabel.setText("MIDI CHANNEL:", juce::dontSendNotification);
  addAndMakeVisible(chanCombo);
  chanCombo.addItem("ALL", 1);
  for (int i = 1; i <= 16; ++i)
    chanCombo.addItem(juce::String(i), i + 1);
  chanAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          engine.getAPVTS(), "midiChannel", chanCombo);

  dashboard = std::make_unique<LibraryDashboard>(e);
  addAndMakeVisible(*dashboard);

  addAndMakeVisible(loadLibBtn);
  loadLibBtn.setButtonText("LOAD SOTERO LIBRARY");
  loadLibBtn.setColour(juce::TextButton::buttonColourId,
                       juce::Colours::darkgreen);
  loadLibBtn.onClick = [this]() {
    chooser = std::make_unique<juce::FileChooser>(
        "Select a Sotero Library (.spsa)...",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.spsa;*.sotero");

    auto chooserFlags = juce::FileBrowserComponent::openMode |
                        juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
      auto file = fc.getResult();
      if (file.existsAsFile()) {
        engine.loadSoteroLibrary(file);
      }
    });
  };
}

void SetupView::resized() {
  auto area = getLocalBounds().reduced(20);
  globalGroup.setBounds(area);

  auto content = area.reduced(20);
  auto topBar = content.removeFromTop(60);

  topBar.removeFromLeft(20);
  chanLabel.setFont(juce::Font(16.0f, juce::Font::bold));
  chanLabel.setBounds(topBar.removeFromLeft(125).reduced(5));
  chanCombo.setBounds(topBar.removeFromLeft(100).reduced(10));

  loadLibBtn.setBounds(content.removeFromBottom(50).reduced(100, 0));

  content.removeFromBottom(20);
  dashboard->setBounds(content);
}

// --- LibraryDashboard Implementation ---
LibraryDashboard::LibraryDashboard(sotero::ISoteroAudioEngine &e)
    : engine(e), volSlider("VOLUME"), panSlider("PAN") {
  addAndMakeVisible(titleLabel);
  titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
  titleLabel.setJustificationType(juce::Justification::centred);
  titleLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);

  addAndMakeVisible(authorLabel);
  authorLabel.setFont(juce::Font(18.0f, juce::Font::plain));
  authorLabel.setJustificationType(juce::Justification::centred);
  authorLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

  addAndMakeVisible(volSlider);
  volSlider.getSlider().setRange(-60.0, 6.0);
  volAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "masterVol", volSlider.getSlider());

  addAndMakeVisible(panSlider);
  panSlider.getSlider().setRange(-1.0, 1.0);
  panAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "pan0", panSlider.getSlider());

  addAndMakeVisible(vuL);
  addAndMakeVisible(vuR);

  startTimerHz(30);
}

void LibraryDashboard::paint(juce::Graphics &g) {
  g.setColour(juce::Colour(0xFF1E1E1E));
  g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2), 15.0f);
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 15.0f, 1.5f);
}

void LibraryDashboard::resized() {
  auto area = getLocalBounds().reduced(20);

  auto topArea = area.removeFromTop(120);
  titleLabel.setBounds(topArea.removeFromTop(50));
  authorLabel.setBounds(topArea);

  area.removeFromTop(20); // Spacer

  auto controlArea = area.removeFromBottom(180);
  int kw = controlArea.getWidth() / 2;
  volSlider.setBounds(controlArea.removeFromLeft(kw).reduced(20));
  panSlider.setBounds(controlArea.reduced(20));

  auto vuArea = area.reduced(100, 20);
  vuL.setBounds(vuArea.removeFromLeft(40));
  vuArea.removeFromLeft(20);
  vuR.setBounds(vuArea.removeFromLeft(40));
}

void LibraryDashboard::timerCallback() {
  titleLabel.setText(engine.getLibraryName(), juce::dontSendNotification);
  authorLabel.setText("by " + engine.getLibraryAuthor(),
                      juce::dontSendNotification);

  vuL.setLevel(engine.getLevelL());
  vuR.setLevel(engine.getLevelR());
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
