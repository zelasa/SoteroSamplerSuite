#pragma once

#include <cstdint>
#include <juce_core/juce_core.h>

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

/**
 * @brief Metadata structure for a single key/sample mapping
 */
struct KeyMapping {
  int midiNote;
  juce::String samplePath;
  juce::String fileName;
  int velocityLow;
  int velocityHigh;
  int chokeGroup;
};

/**
 * @brief High-level metadata for the entire library
 */
struct LibraryMetadata {
  juce::String name;
  juce::String author;
  juce::String description;
  juce::String artworkPath;
  juce::Array<KeyMapping> mappings;
};
} // namespace sotero
