# 2. Criar o Plugin SamplerPlayer
juce_add_plugin(SamplerPlayer
    COMPANY_NAME "SoteropolySamples"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE SPS2
    PLUGIN_CODE Sply
    FORMATS VST3 Standalone
    PRODUCT_NAME "SamplerPlayer"
    BUNDLE_ID "com.soteropolysamples.samplerplayer")

# Binary Data (Assets vivem na raiz da Suite)
juce_add_binary_data(SamplerPlayerAssets
    SOURCES
        ../Assets/logo.png)

juce_generate_juce_header(SamplerPlayer)

# 3. Adicionar arquivos fonte
target_sources(SamplerPlayer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

# 4. Links e Dependências
target_link_libraries(SamplerPlayer PRIVATE
    Common
    SamplerPlayerAssets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_audio_formats
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_cryptography)

# 5. Configurações de compilação
target_compile_definitions(SamplerPlayer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
