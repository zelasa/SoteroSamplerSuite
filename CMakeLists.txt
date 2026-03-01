cmake_minimum_required(VERSION 3.15)

project(SoteroSamplerSuite VERSION 0.1.0)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
endif()

# 1. Encontrar o JUCE
if(WIN32 OR MSVC)
    set(JUCE_PATH_DEFAULT "D:/Luis/OneDrive/Juce")
else()
    set(JUCE_PATH_DEFAULT "/Users/lsantiago/Library/CloudStorage/OneDrive-Pessoal/Juce")
endif()

set(JUCE_PATH "${JUCE_PATH_DEFAULT}" CACHE PATH "Caminho para o framework JUCE" FORCE)

if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "O JUCE nao foi encontrado em: ${JUCE_PATH}. Verifique se o caminho esta correto.")
endif()

add_subdirectory("${JUCE_PATH}" "_juce" EXCLUDE_FROM_ALL)

# 2. Adicionar os Subprojetos
add_subdirectory(Common)
add_subdirectory(SamplerPlayer)
add_subdirectory(SoteroBuilder)
