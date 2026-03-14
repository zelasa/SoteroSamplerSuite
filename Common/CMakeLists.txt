add_library(Common INTERFACE)

target_sources(Common INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/SoteroViews.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/ADSRWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/FilterWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/DynamicsWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/ADSRWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/FilterWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/DynamicsWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/HeaderWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MetadataWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/SculptingWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/AdvancedWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/WaveformWidget.h
)

target_include_directories(Common INTERFACE .)

target_link_libraries(Common INTERFACE 
    juce::juce_core 
    juce::juce_audio_basics 
    juce::juce_audio_processors 
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_cryptography)
