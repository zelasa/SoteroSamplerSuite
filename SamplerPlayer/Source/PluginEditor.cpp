#include "PluginEditor.h"
#include "BinaryData.h"
#include "PluginProcessor.h"


// --- Editor Implementation ---

SamplerPlayerAudioProcessorEditor::SamplerPlayerAudioProcessorEditor(
    SamplerPlayerAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  playerUI = std::make_unique<sotero::SoteroPlayerUI>(audioProcessor);

  auto logoImg = juce::ImageFileFormat::loadFrom(BinaryData::logo_png,
                                                 BinaryData::logo_pngSize);
  playerUI->setLogo(logoImg);

  addAndMakeVisible(*playerUI);

  setSize(1100, 850);
}

SamplerPlayerAudioProcessorEditor::~SamplerPlayerAudioProcessorEditor() {}

void SamplerPlayerAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xFF121212));
}

void SamplerPlayerAudioProcessorEditor::resized() {
  playerUI->setBounds(getLocalBounds());
}
