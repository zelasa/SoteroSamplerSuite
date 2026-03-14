#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sotero {

/**
 * Interface for the shared audio engine state needed by the Player UI.
 * This allows the UI to work with both the SamplerPlayer plugin and the
 * SoteroBuilder app.
 */
class ISoteroAudioEngine {
public:
  virtual ~ISoteroAudioEngine() = default;

  virtual juce::AudioProcessorValueTreeState &getAPVTS() = 0;
  virtual juce::MidiKeyboardState &getKeyboardState() = 0;

  virtual float getLevelL() const = 0;
  virtual float getLevelR() const = 0;

  virtual juce::String getLibraryName() const = 0;
  virtual juce::String getLibraryAuthor() const = 0;
  virtual juce::String getLibraryDescription() const = 0;
  virtual juce::Image getLibraryArtwork() const = 0;
  virtual bool isLibraryLoaded() const = 0;

  // Auditioning
  virtual void auditionMappingStart(int mappingIndex, float velocity) = 0;
  virtual void auditionMappingStop(int mappingIndex) = 0;

  virtual void loadSoteroLibrary(const juce::File &file) = 0;

  // Global monitoring
  virtual int getLastMidiNote() const = 0;
  virtual int getLastMidiVelocity() const = 0;
};

} // namespace sotero
