#pragma once

#include <JuceHeader.h>
#include "../../SamplerPlayer/Source/SoteroSamplerVoice.h"
#include "SoteroMetadata.h"
#include "SoteroEngineInterface.h"
#include "SoteroFormat.h"
#include "SoteroArchive.h"
#include "SoteroLoopEngine.h"
#include <cmath>

namespace sotero {
 
struct SoteroSynthesiser : public juce::Synthesiser {
    void setAPVTS(juce::AudioProcessorValueTreeState* vts) { apvts = vts; }

    void noteOn(int midiChannel, int midiNoteNumber, float velocity) override {
        const int velIdx = (int)(velocity * 127.0f);
        
        // Apply Velocity Curve (0: Soft, 1: Linear, 2: Hard)
        float curvedVelocity = velocity;
        if (apvts != nullptr) {
            int curveType = (int)apvts->getRawParameterValue("velocityCurve")->load();
            if (curveType == 0) curvedVelocity = velocity * velocity;
            else if (curveType == 2) curvedVelocity = std::sqrt(velocity);
        }

        for (int i = 0; i < getNumSounds(); ++i) {
            auto sound = getSound(i);
            if (sound->appliesToNote(midiNoteNumber) && sound->appliesToChannel(midiChannel)) {
                if (auto* s = dynamic_cast<SoteroSamplerSound*>(sound.get())) {
                    if (!s->appliesToVelocity(velIdx))
                        continue;
                }
                
                if (auto* voice = findFreeVoice(sound.get(), midiChannel, midiNoteNumber, true))
                    startVoice(voice, sound.get(), midiChannel, midiNoteNumber, curvedVelocity);
            }
        }
    }

    // Expose protected members for manual auditioning
    using juce::Synthesiser::findFreeVoice;
    using juce::Synthesiser::startVoice;

private:
    juce::AudioProcessorValueTreeState* apvts = nullptr;
};

/**
 * @class SoteroEngine
 * @brief Standalone audio engine that encapsulates the synthesizer and audio processing.
 * This class is designed to be shared by SoteroBuilder and SamplerPlayer.
 */
class SoteroEngine : public ISoteroAudioEngine {
public:
    struct DummyProcessor : public juce::AudioProcessor {
        DummyProcessor()
            : AudioProcessor(BusesProperties().withOutput(
                  "Output", juce::AudioChannelSet::stereo(), true)) {}
        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        const juce::String getName() const override { return "Dummy"; }
        bool acceptsMidi() const override { return true; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 0.0; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return ""; }
        void changeProgramName(int, const juce::String&) override {}
        void getStateInformation(juce::MemoryBlock&) override {}
        void setStateInformation(const void*, int) override {}
    } dummyProcessor;

    // Constructor for Builder: owns its own DummyProcessor + APVTS
    SoteroEngine() {
        formatManager.registerBasicFormats();
        apvts = std::make_unique<juce::AudioProcessorValueTreeState>(
            dummyProcessor, nullptr, "Parameters", createParameterLayout());
        initCommon();
    }

    // Constructor for SamplerPlayer: external processor owns the APVTS
    SoteroEngine(juce::AudioProcessor* host, juce::AudioProcessorValueTreeState::ParameterLayout layout) {
        formatManager.registerBasicFormats();
        apvts = std::make_unique<juce::AudioProcessorValueTreeState>(
            *host, nullptr, "Parameters", std::move(layout));
        initCommon();
    }

    ~SoteroEngine() override = default;

    // --- ISoteroAudioEngine Implementation ---
    juce::AudioProcessorValueTreeState& getAPVTS() override { return *apvts; }
    juce::MidiKeyboardState& getKeyboardState() override { return keyboardState; }
    float getLevelL() const override { return lastLevelL.load(); }
    float getLevelR() const override { return lastLevelR.load(); }
    juce::String getLibraryName() const override { return currentLibName; }
    juce::String getLibraryAuthor() const override { return currentLibAuthor; }
    juce::String getLibraryDescription() const override { return currentLibDesc; }
    juce::Image getLibraryArtwork() const override { return currentArtwork; }
    bool isLibraryLoaded() const override { return libraryLoaded; }
    int getLastMidiNote() const override { return lastMidiNote.load(); }
    int getLastMidiVelocity() const override { return lastMidiVelocity.load(); }
    LibraryMetadata& getMetadata() { return currentMetadata; }
    
    void setBpm(double bpm) override { lastBpm.store(bpm); }
    
    void loadSoteroLibrary(const juce::File& file) override {
        const juce::ScopedLock sl(synthLock);
        currentLibraryFile = file;
        currentMetadata = SoteroArchive::readMetadata(file);
        
        currentLibName = currentMetadata.name;
        currentLibAuthor = currentMetadata.author;
        currentLibDesc = currentMetadata.description;
        
        // Handle Artwork
        if (currentMetadata.artworkPath.isNotEmpty()) {
            auto data = SoteroArchive::extractResource(file, currentMetadata.artworkPath);
            if (data.getSize() > 0)
                currentArtwork = juce::ImageFileFormat::loadFrom(data.getData(), data.getSize());
        }
        
        libraryLoaded = true;
        loopEngine.setLoops(currentMetadata.loops, file);
        rebuildSynth();
    }

    void rebuildSynth() {
        const juce::ScopedLock sl(synthLock);
        juce::ReferenceCountedArray<juce::SynthesiserSound> newSounds;

        std::unique_ptr<juce::FileInputStream> stream;
        if (currentLibraryFile.existsAsFile())
            stream = std::make_unique<juce::FileInputStream>(currentLibraryFile);

        for (int mIndex = 0; mIndex < currentMetadata.mappings.size(); ++mIndex) {
            auto &m = currentMetadata.mappings.getReference(mIndex);
            if (m.samplePath.isNotEmpty()) {
                std::unique_ptr<juce::AudioFormatReader> reader;

                juce::File file(m.samplePath);
                if (file.existsAsFile()) {
                    reader.reset(formatManager.createReaderFor(file));
                } else if (stream != nullptr && stream->openedOk()) {
                    auto data = SoteroArchive::extractResource(*stream, m.samplePath);
                    if (data.getSize() > 0) {
                        auto memStream = std::make_unique<juce::MemoryInputStream>(data, true);
                        reader.reset(formatManager.createReaderFor(std::move(memStream)));
                    }
                }

                if (reader != nullptr) {
                    juce::BigInteger range;
                    range.setBit(m.midiNote);

                    bool isBypassed = (m.micLayer == 0 && layer1Bypassed) || (m.micLayer == 1 && layer2Bypassed);

                    newSounds.add(new sotero::SoteroSamplerSound(
                        m.samplePath, *reader, range, m.midiNote, 0.01, 0.1, 10.0,
                        m.chokeGroup, m.velocityLow, m.velocityHigh, m.sampleStart,
                        m.sampleEnd, m.fadeIn, m.fadeOut, m.volumeMultiplier,
                        m.fineTuneCents, m.micLayer, m.adsrAttack, m.adsrDecay,
                        m.adsrSustain, m.adsrRelease, m.adsrAttackCurve, m.adsrDecayCurve,
                        m.adsrReleaseCurve, m.filterType, m.filterCutoff,
                        m.filterResonance, currentMetadata.enableADSR && !isBypassed,
                        currentMetadata.enableFilter && !isBypassed, m.adsrSustainTime, mIndex));
                }
            }
        }

        synth.clearSounds();
        for (auto *s : newSounds) {
            if (auto* ss = dynamic_cast<sotero::SoteroSamplerSound*>(s))
                synth.addSound(ss);
        }
    }

    void setLayerBypass(int layerIdx, bool bypassed) {
        const juce::ScopedLock sl(synthLock);
        if (layerIdx == 0) layer1Bypassed = bypassed;
        else if (layerIdx == 1) layer2Bypassed = bypassed;
        rebuildSynth();
    }

    void triggerLoop(int slotIndex, bool shouldPlay) override {
        const juce::ScopedLock sl(synthLock);
        loopEngine.triggerLoop(slotIndex, shouldPlay);
    }

    void setLoopCancellationMode(bool active) override {
        const juce::ScopedLock sl(synthLock);
        loopEngine.setCancellationMode(active);
    }

    void auditionMappingStart(int mappingIndex, float velocity) override {
        const juce::ScopedLock sl(synthLock);
        
        for (int i = 0; i < synth.getNumSounds(); ++i) {
            auto sound = synth.getSound(i);
            if (auto* s = dynamic_cast<SoteroSamplerSound*>(sound.get())) {
                if (s->sourceMappingIndex == mappingIndex) {
                    if (auto* voice = synth.findFreeVoice(s, 1, s->midiRootNote, true))
                        synth.startVoice(voice, s, 1, s->midiRootNote, velocity);
                }
            }
        }
    }

    void auditionMappingStop(int mappingIndex) override {
        const juce::ScopedLock sl(synthLock);
        for (int i = 0; i < synth.getNumVoices(); ++i) {
            if (auto* v = dynamic_cast<SoteroSamplerVoice*>(synth.getVoice(i))) {
                if (auto* s = dynamic_cast<const SoteroSamplerSound*>(v->getCurrentlyPlayingSound().get())) {
                    if (s->sourceMappingIndex == mappingIndex) {
                        v->stopNote(0.0f, true);
                    }
                }
            }
        }
    }

    // --- Audio Processing ---
    void prepare(double sampleRate, int samplesPerBlock) {
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = 2;

        synth.setCurrentPlaybackSampleRate(sampleRate);
        for (int i = 0; i < synth.getNumVoices(); ++i) {
            if (auto* v = dynamic_cast<SoteroSamplerVoice*>(synth.getVoice(i)))
                v->prepare(spec);
        }

        masterCompressor.prepare(spec);
        masterReverb.prepare(spec);
        masterToneFilter.prepare(spec);

        updateMasterParameters();
    }

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
        keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
        
        // Track last MIDI activity
        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn()) {
                lastMidiNote.store(msg.getNoteNumber());
                lastMidiVelocity.store(msg.getVelocity());
            }
        }

        const juce::ScopedLock sl(synthLock);

        // --- Loop Engine Processing ---
        loopEngine.processBlock(buffer, midiMessages, lastBpm.load(), spec.sampleRate);

        synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

        // Master FX Chain
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        updateMasterParameters();

        if (apvts->getRawParameterValue("toneEnable")->load() > 0.5f) {
             float tone = apvts->getRawParameterValue("masterTone")->load();
             if (std::abs(tone) > 0.01f)
                 masterToneFilter.process(context);
        }

        if (apvts->getRawParameterValue("masterComp")->load() > 0)
            masterCompressor.process(context);

        if (apvts->getRawParameterValue("revEnable")->load() > 0.5f)
            masterReverb.process(context);

        // Update levels for UI
        lastLevelL.store(buffer.getMagnitude(0, 0, buffer.getNumSamples()));
        lastLevelR.store(buffer.getMagnitude(1, 0, buffer.getNumSamples()));
    }

    void updateMasterParameters() {
        // Sync DSP objects with APVTS
        auto compMode = (int)apvts->getRawParameterValue("masterComp")->load();
        masterCompressor.setThreshold(apvts->getRawParameterValue("compThresh")->load());
        masterCompressor.setRatio(apvts->getRawParameterValue("compRatio")->load());
        masterCompressor.setAttack(apvts->getRawParameterValue("compAttack")->load());
        masterCompressor.setRelease(apvts->getRawParameterValue("compRelease")->load());

        auto& revParams = masterReverb.getParameters();
        juce::dsp::Reverb::Parameters p = revParams;
        p.roomSize = apvts->getRawParameterValue("revSize")->load();
        p.wetLevel = apvts->getRawParameterValue("revMix")->load();
        p.dryLevel = 1.0f - p.wetLevel;
        masterReverb.setParameters(p);

        float tone = apvts->getRawParameterValue("masterTone")->load();
        if (tone > 0) {
            masterToneFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            masterToneFilter.setCutoffFrequency(juce::jmap(tone, 0.0f, 1.0f, 100.0f, 2000.0f));
        } else {
            masterToneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            masterToneFilter.setCutoffFrequency(juce::jmap(std::abs(tone), 0.0f, 1.0f, 20000.0f, 500.0f));
        }
    }

    // --- Sound Management ---
    void clearSounds() {
        const juce::ScopedLock sl(synthLock);
        synth.clearSounds();
    }

    void addSound(SoteroSamplerSound* sound) {
        const juce::ScopedLock sl(synthLock);
        synth.addSound(sound);
    }

    /**
     * @brief Non-destructive parameter update for an existing sound.
     * This avoids the "heavy" rebuild of the entire synth.
     */
    void updateSoundParameters(int mappingIndex, const KeyMapping& m) {
        const juce::ScopedLock sl(synthLock);
        for (int i = 0; i < synth.getNumSounds(); ++i) {
            if (auto* s = dynamic_cast<SoteroSamplerSound*>(synth.getSound(i).get())) {
                if (s->sourceMappingIndex == mappingIndex) {
                    s->adsrParams.attack = m.adsrAttack;
                    s->adsrParams.decay = m.adsrDecay;
                    s->adsrParams.sustain = m.adsrSustain;
                    s->adsrParams.release = m.adsrRelease;
                    s->adsrParams.attackCurve = m.adsrAttackCurve;
                    s->adsrParams.decayCurve = m.adsrDecayCurve;
                    s->adsrParams.releaseCurve = m.adsrReleaseCurve;
                    s->adsrParams.visualSustain = m.adsrSustainTime;
                    
                    s->filterType = m.filterType;
                    s->filterCutoff = m.filterCutoff;
                    s->filterResonance = m.filterResonance;
                    
                    s->volumeMultiplier = m.volumeMultiplier;
                    s->fineTuneCents = m.fineTuneCents;
                    
                    // Note: This won't affect currently playing voices until the next startNote,
                    // unless we also iterate through active voices.
                    for (int j = 0; j < synth.getNumVoices(); ++j) {
                        if (auto* v = dynamic_cast<SoteroSamplerVoice*>(synth.getVoice(j))) {
                            if (v->getCurrentlyPlayingSound().get() == s) {
                                // Real-time voice update
                                v->setMasterPitch(0); // For now
                                // We could add a v->refreshParams() method if needed.
                            }
                        }
                    }
                }
            }
        }
    }

    juce::AudioFormatManager& getFormatManager() { return formatManager; }
    juce::Synthesiser& getSynth() { return synth; }
    juce::CriticalSection& getLock() { return synthLock; }

private:
    void initCommon() {
        synth.setAPVTS(apvts.get());
        for (int i = 0; i < 32; ++i)
            synth.addVoice(new SoteroSamplerVoice());
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
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
            "pan0", "Master Pan", -1.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            "masterPitch", "Master Pitch", -12, 12, 0));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "toneEnable", "Tone Enable", false));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "masterTone", "Master Tone", -1.0f, 1.0f, 0.0f));
        return {params.begin(), params.end()};
    }

    SoteroSynthesiser synth;
    juce::CriticalSection synthLock;
    juce::AudioFormatManager formatManager;
    juce::MidiKeyboardState keyboardState;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
    juce::dsp::ProcessSpec spec;
    
    juce::dsp::Compressor<float> masterCompressor;
    juce::dsp::Reverb masterReverb;
    juce::dsp::StateVariableTPTFilter<float> masterToneFilter;

    // UI state mirror
    std::atomic<float> lastLevelL{0.0f}, lastLevelR{0.0f};
    std::atomic<int> lastMidiNote{-1}, lastMidiVelocity{-1};
    juce::String currentLibName, currentLibAuthor, currentLibDesc;
    juce::Image currentArtwork;
    juce::File currentLibraryFile;
    LibraryMetadata currentMetadata;
    bool libraryLoaded = false;
    bool layer1Bypassed = true;
    bool layer2Bypassed = true;
    std::atomic<double> lastBpm{120.0};
    sotero::SoteroLoopEngine loopEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoteroEngine)
};

} // namespace sotero
