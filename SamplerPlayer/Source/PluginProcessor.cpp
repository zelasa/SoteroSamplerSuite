#include "PluginProcessor.h"
#include "PluginEditor.h"

SamplerPlayerAudioProcessor::SamplerPlayerAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()) {
  formatManager.registerBasicFormats();
  currentLibraryName = "No Library Loaded";
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
const juce::String SamplerPlayerAudioProcessor::getProgramName(int index) {
  return {};
}
void SamplerPlayerAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {}

void SamplerPlayerAudioProcessor::prepareToPlay(double sampleRate,
                                                int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlock;
  spec.numChannels = getTotalNumOutputChannels();

  synth.setCurrentPlaybackSampleRate(sampleRate);

  masterCompressor.prepare(spec);
  masterReverb.prepare(spec);
}

void SamplerPlayerAudioProcessor::releaseResources() {}

bool SamplerPlayerAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void SamplerPlayerAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                               juce::MidiBuffer &midiMessages) {
  keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(),
                                      true);

  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  int targetChannel = (int)*apvts.getRawParameterValue("midiChannel");

  // Velocity Curve Processing
  int curveType = (int)*apvts.getRawParameterValue("velocityCurve");
  juce::MidiBuffer specializedMessages;

  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();

    // Channel Filtering (0 = All)
    if (targetChannel != 0 && msg.getChannel() != targetChannel)
      continue;

    if (msg.isNoteOn()) {
      lastMidiNote.store(msg.getNoteNumber());
      lastMidiVelocity.store(msg.getVelocity());

      float normalizedVel = (float)msg.getVelocity() / 127.0f;
      if (curveType == 0) // Soft
        normalizedVel = std::sqrt(normalizedVel);
      else if (curveType == 2) // Hard
        normalizedVel = std::pow(normalizedVel, 2.0f);

      int velocity = juce::jlimit(1, 127, (int)(normalizedVel * 127.0f));
      auto transformedMsg = juce::MidiMessage::noteOn(
          msg.getChannel(), msg.getNoteNumber(), (juce::uint8)velocity);
      specializedMessages.addEvent(transformedMsg, metadata.samplePosition);
    } else {
      specializedMessages.addEvent(msg, metadata.samplePosition);
    }
  }

  // Render the unified synth
  synth.renderNextBlock(buffer, specializedMessages, 0, buffer.getNumSamples());

  // Update and Apply Master Compressor
  int compType = (int)*apvts.getRawParameterValue("masterComp");
  if (compType > 0) {
    if (compType == 1) { // Light preset as base
      masterCompressor.setThreshold(-20.0f);
      masterCompressor.setRatio(3.0f);
      masterCompressor.setAttack(20.0f);
    } else if (compType == 2) { // Mid preset as base
      masterCompressor.setThreshold(-30.0f);
      masterCompressor.setRatio(3.0f);
      masterCompressor.setAttack(15.0f);
    } else if (compType == 3) { // Hard preset as base
      masterCompressor.setThreshold(-40.0f);
      masterCompressor.setRatio(4.0f);
      masterCompressor.setAttack(12.0f);
    }

    // Override with granular parameters if we want full manual control later,
    // but for now let's use the parameters directly as the primary source.
    masterCompressor.setThreshold(*apvts.getRawParameterValue("compThresh"));
    masterCompressor.setRatio(*apvts.getRawParameterValue("compRatio"));
    masterCompressor.setAttack(*apvts.getRawParameterValue("compAttack"));
    masterCompressor.setRelease(*apvts.getRawParameterValue("compRelease"));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    masterCompressor.process(context);
  }

  // --- Master Reverb ---
  bool revEnabled = (bool)*apvts.getRawParameterValue("revEnable");
  if (revEnabled) {
    reverbParams.roomSize = *apvts.getRawParameterValue("revSize");
    reverbParams.damping = 0.5f; // Fixed for now
    reverbParams.wetLevel = *apvts.getRawParameterValue("revMix");
    reverbParams.dryLevel =
        1.0f - (*apvts.getRawParameterValue("revMix") * 0.5f);
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;

    // Low-pass/High-pass behavior can be simulated by room size/damping in
    // juce::Reverb
    masterReverb.setParameters(reverbParams);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    masterReverb.process(context);
  }

  // --- Master Volume ---
  float masterGain = juce::Decibels::decibelsToGain(
      (float)*apvts.getRawParameterValue("masterVol"));
  buffer.applyGain(masterGain);

  // --- Master Level Tracking ---
  if (buffer.getNumSamples() > 0) {
    lastMasterLevelL.store(buffer.getMagnitude(0, 0, buffer.getNumSamples()));
    if (buffer.getNumChannels() > 1)
      lastMasterLevelR.store(buffer.getMagnitude(1, 0, buffer.getNumSamples()));
    else
      lastMasterLevelR.store(lastMasterLevelL.load());
  }
}

bool SamplerPlayerAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor *SamplerPlayerAudioProcessor::createEditor() {
  return new SamplerPlayerAudioProcessorEditor(*this);
}

void SamplerPlayerAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void SamplerPlayerAudioProcessor::setStateInformation(const void *data,
                                                      int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));
  if (xmlState != nullptr)
    if (xmlState->hasTagName(apvts.state.getType()))
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout
SamplerPlayerAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "midiNote", "Trigger Note", 0, 127, 60));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "midiChannel", "Midi Channel", 0, 16, 0)); // 0 = All
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "velocityCurve", "Velocity Curve", 0, 2, 1)); // 0:Soft, 1:Linear, 2:Hard

  // --- Compressor Parameters ---
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "masterComp", "Master Compressor Mode", 0, 3,
      0)); // 0:Off, 1:Light, 2:Mid, 3:Hard
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compThresh", "Threshold", -60.0f, 0.0f, -20.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compRatio", "Ratio", 1.0f, 10.0f, 3.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compAttack", "Attack", 1.0f, 100.0f, 20.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "compRelease", "Release", 10.0f, 500.0f, 100.0f));

  // --- Reverb Parameters ---
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      "revEnable", "Reverb Enable", false));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "revType", "Reverb Type", 0, 3, 0)); // 0:Room, 1:Hall, 2:Plate, 3:Stadium
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "revSize", "Room Size", 0.0f, 1.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "revMix", "Mix", 0.0f, 1.0f, 0.3f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "masterVol", "Master Volume", -60.0f, 6.0f, 0.0f));

  for (int i = 0; i < 3; ++i) {
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "vol" + juce::String(i), "Volume " + juce::String(i + 1), -60.0f, 6.0f,
        0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pan" + juce::String(i), "Pan " + juce::String(i + 1), -1.0f, 1.0f,
        0.0f));
  }

  return {params.begin(), params.end()};
}

void SamplerPlayerAudioProcessor::loadSoteroLibrary(const juce::File &file) {
  if (!file.existsAsFile())
    return;

  currentLibraryFile = file;

  auto metadata = sotero::SoteroArchive::readMetadata(file);
  if (metadata.name.isEmpty())
    return;

  currentLibraryName = metadata.name;
  currentLibraryAuthor = metadata.author;

  synth.clearSounds();
  synth.clearVoices();

  // Add 16 voices for polyphony
  for (int i = 0; i < 16; ++i)
    synth.addVoice(new sotero::SoteroSamplerVoice());

  for (const auto &mapping : metadata.mappings) {
    auto sampleData =
        sotero::SoteroArchive::extractResource(file, mapping.samplePath);
    if (sampleData.getSize() > 0) {
      auto reader = formatManager.createReaderFor(
          std::make_unique<juce::MemoryInputStream>(sampleData, false));

      if (reader != nullptr) {
        juce::BigInteger range;
        range.setBit(mapping.midiNote);

        synth.addSound(new sotero::SoteroSamplerSound(
            mapping.samplePath, *reader, range, mapping.midiNote, 0.01, 0.1,
            10.0, mapping.chokeGroup, mapping.velocityLow,
            mapping.velocityHigh));

        delete reader;
      }
    }
  }

  return true;
}

bool SamplerPlayerAudioProcessor::loadTrackSample(int index,
                                                  const juce::File &file) {
  return false; // Deprecated
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new SamplerPlayerAudioProcessor();
}
