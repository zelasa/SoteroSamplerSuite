add_library(Common INTERFACE)

target_include_directories(Common INTERFACE .)

target_link_libraries(Common INTERFACE juce::juce_core)
