# 2. Criar a ferramenta Standalone SoteroBuilder
juce_add_gui_app(SoteroBuilder
    COMPANY_NAME "SoteropolySamples"
    PRODUCT_NAME "SoteroBuilder"
    BUNDLE_ID "com.soteropolysamples.soterobuilder")

# Assets compartilhados
juce_add_binary_data(SoteroBuilderAssets
    SOURCES
        ../Assets/logo.png)

juce_generate_juce_header(SoteroBuilder)

# 3. Arquivos Fonte
target_sources(SoteroBuilder PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/SampleRegion.cpp
    Source/SampleRegion.h
    Source/LibraryController.cpp)

target_include_directories(SoteroBuilder PRIVATE 
    Source
    ../SamplerPlayer/Source)

# 4. Links e Dependências
target_link_libraries(SoteroBuilder PRIVATE
    Common
    SoteroBuilderAssets
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_audio_formats
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_cryptography)
