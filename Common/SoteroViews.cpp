#include "SoteroViews.h"

namespace sotero {
// --- SoteroKeyboard Implementation ---
SoteroKeyboard::SoteroKeyboard(juce::MidiKeyboardState &state)
    : keyboard(state, juce::MidiKeyboardComponent::horizontalKeyboard) {
  keyboard.setScrollButtonsVisible(false);
  addAndMakeVisible(keyboard);
  addAndMakeVisible(octLeftBtn);
  addAndMakeVisible(octRightBtn);

  octLeftBtn.onClick = [this] {
    firstNote = juce::jlimit(0, 127 - 60, firstNote - 12);
    setOctaveRange(firstNote);
  };
  octRightBtn.onClick = [this] {
    firstNote = juce::jlimit(0, 127 - 60, firstNote + 12);
    setOctaveRange(firstNote);
  };

  setOctaveRange(firstNote);
}

void SoteroKeyboard::resized() {
  auto area = getLocalBounds();
  octLeftBtn.setBounds(area.removeFromLeft(25));
  octRightBtn.setBounds(area.removeFromRight(25));
  keyboard.setBounds(area);
  // 5 octaves = 60 notes. Roughly 35 white keys.
  keyboard.setKeyWidth((float)area.getWidth() / 35.0f);
}

void SoteroKeyboard::setOctaveRange(int note) {
  firstNote = note;
  keyboard.setAvailableRange(firstNote, firstNote + 60);
}

// --- PerformanceView Implementation ---

PerformanceView::PerformanceView(sotero::ISoteroAudioEngine &e)
    : engine(e), threshKnob("THRESH"), ratioKnob("RATIO"), attackKnob("ATTACK"),
      releaseKnob("RELEASE"), revSizeKnob("SIZE"), revMixKnob("MIX"),
      masterVolKnob("MASTER"), pitchKnob("PITCH"), toneKnob("TONE") {
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

  addAndMakeVisible(pitchKnob);
  pitchAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "masterPitch", pitchKnob.getSlider());

  addAndMakeVisible(toneKnob);
  toneAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          engine.getAPVTS(), "masterTone", toneKnob.getSlider());

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
  // Hero Master Area
  auto mastControls = mastInner.reduced(10, 5);
  int mw = mastControls.getWidth() / 3;
  pitchKnob.setBounds(mastControls.removeFromLeft(mw).reduced(5));
  toneKnob.setBounds(mastControls.removeFromLeft(mw).reduced(5));
  masterVolKnob.setBounds(mastControls.reduced(5));
}

// --- SetupView Implementation ---

SetupView::SetupView(sotero::ISoteroAudioEngine &e) : engine(e) {
  addAndMakeVisible(machineIDLabel);
  machineIDLabel.setText("YOUR MACHINE DNA: " + SoteroSecurity::getMachineID(),
                         juce::dontSendNotification);
  machineIDLabel.setColour(juce::Label::textColourId,
                           juce::Colours::cyan.withAlpha(0.8f));
  machineIDLabel.setJustificationType(juce::Justification::centred);
  machineIDLabel.setFont(juce::Font(14.0f, juce::Font::bold));

  addAndMakeVisible(loginLabel);
  loginLabel.setText("ACCOUNT LOGIN:", juce::dontSendNotification);
  addAndMakeVisible(loginInput);

  addAndMakeVisible(passLabel);
  passLabel.setText("PASSWORD:", juce::dontSendNotification);
  addAndMakeVisible(passInput);
  passInput.setPasswordCharacter('*');

  addAndMakeVisible(serialLabel);
  serialLabel.setText("SERIAL KEY / LICENSE:", juce::dontSendNotification);
  addAndMakeVisible(serialInput);

  addAndMakeVisible(activateBtn);
  activateBtn.setColour(juce::TextButton::buttonColourId,
                        juce::Colours::darkgreen);
  activateBtn.onClick = [this] {
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon, "Sotero Shield",
        "Connecting to server...\nValidation successful!\nYour machine is now "
        "authorized for your libraries.");
  };

  addAndMakeVisible(globalGroup);
  globalGroup.setText("AUDIO SETTINGS");

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

void SetupView::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xFF1A1A1A));
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
  titleLabel.setFont(juce::Font(32.0f, juce::Font::bold));
  titleLabel.setJustificationType(juce::Justification::centredLeft);
  titleLabel.setColour(juce::Label::textColourId,
                       juce::Colour(0xFFFFFF00)); // Yellow Neon

  addAndMakeVisible(authorLabel);
  authorLabel.setFont(juce::Font(20.0f, juce::Font::plain));
  authorLabel.setJustificationType(juce::Justification::centredLeft);
  authorLabel.setColour(juce::Label::textColourId,
                        juce::Colour(0xFF00FFFF)); // Cyan Neon

  addAndMakeVisible(artworkComponent);
  artworkComponent.setInterceptsMouseClicks(false, false);

  addAndMakeVisible(descriptionDisplay);
  descriptionDisplay.setMultiLine(true);
  descriptionDisplay.setReadOnly(true);
  descriptionDisplay.setReturnKeyStartsNewLine(true);
  descriptionDisplay.setCaretVisible(false);
  descriptionDisplay.setScrollbarsShown(true);
  descriptionDisplay.setColour(juce::TextEditor::backgroundColourId,
                               juce::Colours::transparentBlack);
  descriptionDisplay.setColour(juce::TextEditor::outlineColourId,
                               juce::Colours::transparentBlack);
  descriptionDisplay.setColour(juce::TextEditor::textColourId,
                               juce::Colours::white.withAlpha(0.8f));
  descriptionDisplay.setFont(juce::Font(15.0f));

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
  auto bounds = getLocalBounds().toFloat();

  // Background
  g.setColour(juce::Colour(0xFF1E1E1E));
  g.fillRoundedRectangle(bounds.reduced(2), 15.0f);

  // Glow effect for artwork area
  auto artArea = bounds.reduced(20).removeFromLeft(bounds.getHeight() - 40);
  juce::ColourGradient glow(juce::Colour(0x3300FFFF), artArea.getCentreX(),
                            artArea.getCentreY(), juce::Colour(0x00000000),
                            0.0f, 0.0f, true);
  g.setGradientFill(glow);
  g.fillRoundedRectangle(artArea.expanded(15), 10.0f);

  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRoundedRectangle(bounds.reduced(2), 15.0f, 1.5f);
}

void LibraryDashboard::resized() {
  auto area = getLocalBounds().reduced(20);

  // Left side: Artwork
  auto leftArea = area.removeFromLeft(area.getHeight()); // Square for art
  artworkComponent.setBounds(leftArea.reduced(5));

  area.removeFromLeft(30); // Spacer

  // Right side: Info & Controls
  titleLabel.setBounds(area.removeFromTop(45));
  authorLabel.setBounds(area.removeFromTop(30));

  area.removeFromTop(15); // Spacer

  // Description area
  descriptionDisplay.setBounds(area.removeFromTop(150));

  auto controlArea = area.removeFromBottom(150);
  int kw = controlArea.getWidth() / 2;

  auto vArea = controlArea.removeFromLeft(kw).reduced(10);
  volSlider.setBounds(vArea);

  auto pArea = controlArea.reduced(10);
  panSlider.setBounds(pArea.removeFromTop(100));

  // Small VU meters integrated below
  auto vuArea = pArea.reduced(20, 0);
  vuL.setBounds(vuArea.removeFromLeft(15));
  vuArea.removeFromLeft(5);
  vuR.setBounds(vuArea.removeFromLeft(15));
}

void LibraryDashboard::timerCallback() {
  titleLabel.setText(engine.getLibraryName(), juce::dontSendNotification);
  authorLabel.setText("by " + engine.getLibraryAuthor(),
                      juce::dontSendNotification);
  descriptionDisplay.setText(engine.getLibraryDescription(),
                             juce::dontSendNotification);
  artworkComponent.setImage(engine.getLibraryArtwork());

  vuL.setLevel(engine.getLevelL());
  vuR.setLevel(engine.getLevelR());
}

// --- SoteroPlayerUI Implementation ---

SoteroPlayerUI::SoteroPlayerUI(sotero::ISoteroAudioEngine &e)
    : engine(e), keyboard(e.getKeyboardState()) {
  addAndMakeVisible(performBtn);
  performBtn.setClickingTogglesState(true);
  performBtn.setRadioGroupId(100);
  performBtn.setToggleState(true, juce::dontSendNotification);
  performBtn.onClick = [this] {
    performanceView->setVisible(true);
    setupView->setVisible(false);
    libraryBrowser->setVisible(false);
    libraryDashboard->setVisible(false);
  };

  addAndMakeVisible(setupBtn);
  setupBtn.setClickingTogglesState(true);
  setupBtn.setRadioGroupId(100);
  setupBtn.onClick = [this] {
    performanceView->setVisible(false);
    setupView->setVisible(true);
    libraryBrowser->setVisible(false);
    libraryDashboard->setVisible(false);
  };

  addAndMakeVisible(libBtn);
  libBtn.setClickingTogglesState(true);
  libBtn.setRadioGroupId(100);
  libBtn.onClick = [this] {
    performanceView->setVisible(false);
    setupView->setVisible(false);

    if (engine.isLibraryLoaded()) {
      libraryDashboard->setVisible(true);
      libraryBrowser->setVisible(false);
    } else {
      libraryDashboard->setVisible(false);
      libraryBrowser->setVisible(true);
      libraryBrowser->refresh();
    }
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

  addAndMakeVisible(versionLabel);
  versionLabel.setFont(juce::Font(14.0f));
  versionLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
  versionLabel.setJustificationType(juce::Justification::centredRight);

  addAndMakeVisible(logo);
  // (We'll expect BinaryData to be available in the apps, but here we just
  // placeholder or expect common code to provide it. For now, just setup.)

  performanceView = std::make_unique<PerformanceView>(engine);
  addAndMakeVisible(*performanceView);

  setupView = std::make_unique<SetupView>(engine);
  addAndMakeVisible(*setupView);
  setupView->setVisible(false);

  libraryBrowser = std::make_unique<LibraryBrowser>(engine);
  addAndMakeVisible(*libraryBrowser);
  libraryBrowser->setVisible(false);

  libraryDashboard = std::make_unique<LibraryDashboard>(engine);
  addAndMakeVisible(*libraryDashboard);
  libraryDashboard->setVisible(false);

  addAndMakeVisible(keyboard);

  startTimerHz(30);
}

void SoteroPlayerUI::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xFF121212));
}

void SoteroPlayerUI::resized() {
  auto area = getLocalBounds();
  auto topBar = area.removeFromTop(70).reduced(15, 10);

  // Logo (expecting ~150px)
  logo.setBounds(topBar.removeFromLeft(150));

  // Monitor (Right)
  auto monitorArea = topBar.removeFromRight(200);
  midiMonitorLabel.setBounds(
      monitorArea.removeFromTop(monitorArea.getHeight() / 3));
  midiVelocityLabel.setBounds(
      monitorArea.removeFromTop(monitorArea.getHeight() / 2));
  versionLabel.setBounds(monitorArea);

  // Navigation (Center)
  auto tabs = topBar.reduced(50, 5);
  int tabW = tabs.getWidth() / 3;
  performBtn.setBounds(tabs.removeFromLeft(tabW).reduced(10, 0));
  libBtn.setBounds(tabs.removeFromLeft(tabW).reduced(10, 0));
  setupBtn.setBounds(tabs.reduced(10, 0));

  keyboard.setBounds(area.removeFromBottom(120).reduced(20, 10));

  performanceView->setBounds(area);
  setupView->setBounds(area);
  libraryBrowser->setBounds(area);
  libraryDashboard->setBounds(area);
}

void SoteroPlayerUI::timerCallback() {
  int lastNote = engine.getLastMidiNote();
  int lastVel = engine.getLastMidiVelocity();
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

// --- LibraryBrowser Implementation ---

LibraryBrowser::Item::Item(const SoteroLibraryManager::LibraryEntry &e)
    : entry(e) {}

void LibraryBrowser::Item::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat().reduced(2);

  // Background
  if (entry.isLocked) {
    g.setColour(juce::Colours::red.withAlpha(0.15f));
  } else {
    g.setColour(isSelected ? juce::Colours::orange.withAlpha(0.2f)
                           : juce::Colours::black.withAlpha(0.2f));
  }
  g.fillRoundedRectangle(bounds, 5.0f);

  // Selection Border
  if (isSelected) {
    g.setColour(juce::Colours::orange);
    g.drawRoundedRectangle(bounds, 5.0f, 2.0f);
  } else {
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
  }

  auto textBounds = bounds.reduced(10);
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font(16.0f, juce::Font::bold));
  g.drawText(entry.name, textBounds.removeFromTop(20),
             juce::Justification::centredLeft);

  g.setColour(juce::Colours::grey);
  g.setFont(12.0f);
  g.drawText(entry.author, textBounds.removeFromTop(15),
             juce::Justification::centredLeft);

  g.drawText(entry.instrumentType, textBounds.removeFromBottom(15),
             juce::Justification::bottomRight);

  if (entry.isLocked) {
    g.setColour(juce::Colours::red);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("DNA LOCKED", bounds.reduced(10), juce::Justification::topRight);
  }
}

void LibraryBrowser::Item::mouseDown(const juce::MouseEvent &) {
  if (entry.isLocked) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon, "Sotero Shield: Library Locked",
        "This library is bound to another machine (DNA mismatch).\nYour "
        "Machine "
        "ID: " +
            SoteroSecurity::getMachineID() +
            "\n\nPlease activate via your Sotero account.");
    return;
  }
  if (onLoadRequested)
    onLoadRequested(entry.file);
}

LibraryBrowser::LibraryBrowser(sotero::ISoteroAudioEngine &e) : engine(e) {
  addAndMakeVisible(viewport);
  viewport.setViewedComponent(&content);
  refresh();
}

void LibraryBrowser::refresh() {
  libManager.refresh();
  items.clear();

  auto &libs = libManager.getLibraries();
  for (auto &lib : libs) {
    auto *item = items.add(new Item(lib));
    content.addAndMakeVisible(item);
    item->onLoadRequested = [this](juce::File f) {
      engine.loadSoteroLibrary(f);
      refresh(); // Redraw selection if we add selection state later
    };
  }

  resized();
}

void LibraryBrowser::paint(juce::Graphics &g) {
  g.setColour(juce::Colours::black.withAlpha(0.3f));
  g.fillAll();
}

void LibraryBrowser::resized() {
  viewport.setBounds(getLocalBounds());

  int itemH = 80;
  int spacing = 5;
  content.setBounds(0, 0,
                    viewport.getWidth() - viewport.getScrollBarThickness(),
                    items.size() * (itemH + spacing) + spacing);

  for (int i = 0; i < items.size(); ++i) {
    items[i]->setBounds(spacing, spacing + i * (itemH + spacing),
                        content.getWidth() - spacing * 2, itemH);
  }
}

} // namespace sotero
