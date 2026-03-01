#pragma once

#include "SoteroMetadata.h"
#include <juce_core/juce_core.h>

namespace sotero {
/**
 * @class SoteroArchive
 * @brief Handles the creation and reading of .spsa / .sotero containers.
 */
class SoteroArchive {
public:
  /**
   * @brief Writes a complete library to a file, including binary assets.
   */
  static bool write(const juce::File &outputFile, LibraryMetadata &metadata) {
    juce::FileOutputStream stream(outputFile);
    if (!stream.openedOk())
      return false;

    stream.setPosition(0);
    stream.truncate();

    // 1. Prepare Binary Payload & Update Offsets
    juce::MemoryBlock payload;

    // Handle Artwork
    if (!metadata.artworkPath.isEmpty()) {
      juce::File artFile(metadata.artworkPath);
      if (artFile.existsAsFile()) {
        size_t offset = payload.getSize();
        size_t size = (size_t)artFile.getSize();
        artFile.loadFileAsData(payload);
        metadata.artworkPath = juce::String(offset) + ":" +
                               juce::String(size); // Store as offset:size
      }
    }

    // Handle Samples
    for (auto &mapping : metadata.mappings) {
      if (!mapping.samplePath.isEmpty()) {
        juce::File sFile(mapping.samplePath);
        if (sFile.existsAsFile()) {
          // Ensure 8-byte alignment for ARM performance
          while (payload.getSize() % 8 != 0) {
            const char zero = 0;
            payload.append(&zero, 1);
          }

          size_t offset = payload.getSize();
          size_t size = (size_t)sFile.getSize();
          sFile.loadFileAsData(payload);

          // Internal reference: "OFFSET:SIZE"
          mapping.samplePath = juce::String(offset) + ":" + juce::String(size);
        }
      }
    }

    // 2. Serialize Metadata with internal offsets
    juce::String xmlString = SoteroMetadataHandler::toXmlString(metadata);
    juce::MemoryBlock xmlData;
    xmlData.append(xmlString.toRawUTF8(), xmlString.getNumBytesAsUTF8());

    // 3. Write Header
    SoteroHeader header;
    memcpy(header.magic, constants::kMagicNumber, 8);
    header.version = constants::kCurrentVersion;
    header.xmlMetadataSize = (uint32_t)xmlData.getSize();
    header.encryptionFlags = 0;

    stream.write(&header, sizeof(SoteroHeader));

    // 4. Write XML Manifest
    stream.write(xmlData.getData(), xmlData.getSize());

    // 5. Write Binary Payload
    stream.write(payload.getData(), payload.getSize());

    stream.flush();
    return true;
  }

  /**
   * @brief Extracts a binary resource from an archive.
   */
  static juce::MemoryBlock extractResource(const juce::File &archiveFile,
                                           const juce::String &internalPath) {
    auto parts = juce::StringArray::fromTokens(internalPath, ":", "");
    if (parts.size() < 2)
      return {};

    size_t offset = (size_t)parts[0].getLargeIntValue();
    size_t size = (size_t)parts[1].getLargeIntValue();

    juce::FileInputStream stream(archiveFile);
    if (!stream.openedOk())
      return {};

    SoteroHeader header;
    stream.read(&header, sizeof(SoteroHeader));

    // Byte offset in file = Header + XML + internal offset
    int64_t absoluteOffset =
        sizeof(SoteroHeader) + header.xmlMetadataSize + offset;
    stream.setPosition(absoluteOffset);

    juce::MemoryBlock data;
    stream.readIntoMemoryBlock(data, (int)size);
    return data;
  }

  /**
   * @brief Reads the metadata from an archive.
   */
  static LibraryMetadata readMetadata(const juce::File &inputFile) {
    juce::FileInputStream stream(inputFile);
    if (!stream.openedOk())
      return {};

    SoteroHeader header;
    if (stream.read(&header, sizeof(SoteroHeader)) < sizeof(SoteroHeader))
      return {};

    // Validate Magic Number
    if (memcmp(header.magic, constants::kMagicNumber, 8) != 0)
      return {};

    // Read XML Manifest
    juce::MemoryBlock xmlData;
    if (stream.readIntoMemoryBlock(xmlData, (int)header.xmlMetadataSize) <
        header.xmlMetadataSize)
      return {};

    juce::String xmlString = xmlData.toString();
    return SoteroMetadataHandler::fromXmlString(xmlString);
  }
};
} // namespace sotero
