add_library(Common INTERFACE)

target_include_directories(Common INTERFACE .)

target_link_libraries(Common INTERFACE 
    juce::juce_core 
    juce::juce_audio_basics 
    juce::juce_audio_processors 
    juce::juce_gui_basics)
