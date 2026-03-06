#pragma once

#include <cstdint>
#include <juce_core/juce_core.h>
#include <memory>

/**
 * @file SoteroFormat.h
 * @brief Core specification and structures for the .spsa / .sotero format.
 *
 * This file defines the binary layout and constants for the Soteropoly Samples
 * Archive. Implementation follows international standards for cross-platform
 * compatibility.
 */

namespace sotero {
/**
 * @brief Constants for the Sotero Format
 */
namespace constants {
static constexpr char kMagicNumber[8] = {'S', 'O', 'T', 'E',
                                         'R', 'O', 'P', 'Y'};
static constexpr uint32_t kCurrentVersion = 1;
static constexpr size_t kHeaderSize =
    20; // 8 (Magic) + 4 (Ver) + 4 (XML Size) + 4 (Flags)
} // namespace constants

/**
 * @brief Fixed-width binary header for .spsa / .sotero files.
 * Enforced as Little-Endian in the implementation.
 */
#pragma pack(push, 1)
struct SoteroHeader {
  char magic[8];            ///< Must be "SOTEROPY"
  uint32_t version;         ///< Format version
  uint32_t xmlMetadataSize; ///< Length of the XML manifesto block
  uint32_t encryptionFlags; ///< 0 = None, 1 = DNA Encrypted
};
#pragma pack(pop)

/** Metadata for a single sample mapping */
struct KeyMapping {
  int midiNote = -1;
  juce::String samplePath;
  juce::String fileName;
  int velocityLow = 0;
  int velocityHigh = 127;
  int chokeGroup = 0;

  // New non-destructive metadata
  int64_t sampleStart = 0;
  int64_t sampleEnd = 0;
  int64_t fadeIn = 0;
  int64_t fadeOut = 0;
  float volumeMultiplier = 1.0f;
  float fineTuneCents = 0.0f;
  int micLayer = 0; // 0 = Layer 1 (Mic 1), 1 = Layer 2 (Mic 2)

  // Sculpting Parameters (Per-mapping)
  float adsrAttack = 0.01f;
  float adsrDecay = 0.1f;
  float adsrSustain = 1.0f;
  float adsrRelease = 0.1f;

  int filterType = 0; // 0 = None, 1 = LP, 2 = HP, 3 = BP
  float filterCutoff = 20000.0f;
  float filterResonance = 1.0f;
};

/** Metadata for a MIDI Loop slot */
struct LoopMapping {
  int slotIndex = 0; // 0-35 (Abas A, B, C)
  juce::String midiPath;
  juce::String name;

  bool syncToHost = true;
  float tempoMultiplier = 1.0f; // 0.5, 1.0, 2.0
};

/** Metadata for the entire library */
struct LibraryMetadata {
  juce::String name;
  juce::String author; // also "creator"
  juce::String description;
  juce::String creationDate;
  juce::String instrumentType;
  juce::String artworkPath;

  // Compilation Toggles (Enabled in Player)
  bool enableCompressor = false;
  bool enableEQ = false;
  bool enableReverb = false;
  bool enablePunch = false;
  bool enableADSR = true;
  bool enableFilter = true;

  // Loop Global Settings
  bool loopCancellationMode = false;

  juce::Array<KeyMapping> mappings;
  juce::Array<LoopMapping> loops;
};
} // namespace sotero
