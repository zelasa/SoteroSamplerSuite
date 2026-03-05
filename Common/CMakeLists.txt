add_library(Common INTERFACE)

target_sources(Common INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/SoteroViews.cpp
)

target_include_directories(Common INTERFACE .)

target_link_libraries(Common INTERFACE 
    juce::juce_core 
    juce::juce_audio_basics 
    juce::juce_audio_processors 
    juce::juce_gui_basics
    juce::juce_gui_extra)
