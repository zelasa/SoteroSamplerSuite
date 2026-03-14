#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SoteroArchive.h"

SamplerPlayerAudioProcessor::SamplerPlayerAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)) {
  // Engine owns APVTS with this processor as host (required for DAW preset saving)
  engine = std::make_unique<sotero::SoteroEngine>(this, createParameterLayout());
}

SamplerPlayerAudioProcessor::~SamplerPlayerAudioProcessor() {}

const juce::String SamplerPlayerAudioProcessor::getName() const {
  return "SamplerPlayer";
}

bool SamplerPlayerAudioProcessor::acceptsMidi() const { return true; }
bool SamplerPlayerAudioProcessor::producesMidi() const { return false; }
bool SamplerPlayerAudioProcessor::isMidiEffect() const { return false; }
double SamplerPlayerAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int SamplerPlayerAudioProcessor::getNumPrograms() { return 1; }
int SamplerPlayerAudioProcessor::getCurrentProgram() { return 0; }
void SamplerPlayerAudioProcessor::setCurrentProgram(int index) {}
const juce::String SamplerPlayerAudioProcessor::getProgramName(int index) { return {}; }
void SamplerPlayerAudioProcessor::changeProgramName(int index, const juce::String &newName) {}

void SamplerPlayerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  engine->prepare(sampleRate, samplesPerBlock);
}

void SamplerPlayerAudioProcessor::releaseResources() {}

bool SamplerPlayerAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;
  return true;
}

void SamplerPlayerAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                               juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // Process loops
  double bpm = getPlayHead() != nullptr
                   ? (*getPlayHead()->getPosition()).getBpm().orFallback(120.0)
                   : 120.0;
  loopEngine.processBlock(buffer, midiMessages, bpm);

  // Delegate all synth + effects processing to the engine
  engine->process(buffer, midiMessages);
}

bool SamplerPlayerAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor *SamplerPlayerAudioProcessor::createEditor() {
  return new SamplerPlayerAudioProcessorEditor(*this);
}

void SamplerPlayerAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  auto state = engine->getAPVTS().copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void SamplerPlayerAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
  if (xmlState != nullptr)
    if (xmlState->hasTagName(engine->getAPVTS().state.getType()))
      engine->getAPVTS().replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout
SamplerPlayerAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "midiNote", "Trigger Note", 0, 127, 60));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "midiChannel", "Midi Channel", 0, 16, 0));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "velocityCurve", "Velocity Curve", 0, 2, 1));

  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "masterComp", "Master Compressor Mode", 0, 3, 0));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compThresh", "Threshold", -60.0f, 0.0f, -20.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compRatio", "Ratio", 1.0f, 10.0f, 3.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compAttack", "Attack", 1.0f, 100.0f, 20.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compRelease", "Release", 10.0f, 500.0f, 100.0f));

  params.push_back(std::make_unique<juce::AudioParameterBool>(
      "revEnable", "Reverb Enable", false));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "revType", "Reverb Type", 0, 3, 0));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "revSize", "Room Size", 0.0f, 1.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "revMix", "Mix", 0.0f, 1.0f, 0.0f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "masterVol", "Master Volume", -60.0f, 6.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "masterPitch", "Master Pitch", -12.0f, 12.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "masterTone", "Master Tone", -1.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      "toneEnable", "Tone Enable", false));

  for (int i = 0; i < 3; ++i) {
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "vol" + juce::String(i), "Volume " + juce::String(i + 1), -60.0f, 6.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pan" + juce::String(i), "Pan " + juce::String(i + 1), -1.0f, 1.0f, 0.0f));
  }

  return {params.begin(), params.end()};
}

void SamplerPlayerAudioProcessor::loadSoteroLibrary(const juce::File &file) {
  currentLibraryFile = file;
  engine->loadSoteroLibrary(file);
}

void SamplerPlayerAudioProcessor::auditionMappingStart(int mappingIndex, float velocity) {
  engine->auditionMappingStart(mappingIndex, velocity);
}

void SamplerPlayerAudioProcessor::auditionMappingStop(int mappingIndex) {
  engine->auditionMappingStop(mappingIndex);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new SamplerPlayerAudioProcessor();
}
