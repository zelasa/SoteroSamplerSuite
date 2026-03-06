#pragma once

#include "SoteroMetadata.h"
#include "SoteroSecurity.h"
#include <juce_core/juce_core.h>

namespace sotero {
/**
 * @class SoteroArchive
 * @brief Handles the creation and reading of .spsa / .sotero containers.
 */
/**
 * @class SoteroEncryptedStream
 * @brief Anti-piracy wrapper for InputStream.
 */
class SoteroEncryptedStream : public juce::InputStream {
public:
  SoteroEncryptedStream(juce::InputStream *source, const juce::String &key)
      : sourceStream(source), userKey(key) {}

  int64_t getTotalLength() override { return sourceStream->getTotalLength(); }
  int64_t getPosition() override { return sourceStream->getPosition(); }
  bool setPosition(int64_t newPos) override {
    return sourceStream->setPosition(newPos);
  }
  bool isExhausted() override { return sourceStream->isExhausted(); }

  int read(void *destBuffer, int maxBytesToRead) override {
    int64_t startPos = sourceStream->getPosition();
    int bytesRead = sourceStream->read(destBuffer, maxBytesToRead);
    if (bytesRead > 0 && !userKey.isEmpty()) {
      uint8_t *data = static_cast<uint8_t *>(destBuffer);
      for (int i = 0; i < bytesRead; ++i) {
        data[i] ^= (uint8_t)userKey[(startPos + i) % userKey.length()];
      }
    }
    return bytesRead;
  }

private:
  std::unique_ptr<juce::InputStream> sourceStream;
  juce::String userKey;
};

public:
/**
 * @brief Writes a complete library to a file, including binary assets.
 */
static bool write(const juce::File &outputFile, LibraryMetadata metadata,
                  const juce::File &sourceArchive = juce::File{}) {
  // 1. Prepare Binary Payload & Update Offsets
  juce::MemoryBlock payload;

  // Handle Artwork
  if (!metadata.artworkPath.isEmpty()) {
    juce::MemoryBlock data;

    bool isInternalArt = false;
    if (metadata.artworkPath.contains(":")) {
      auto parts = juce::StringArray::fromTokens(metadata.artworkPath, ":", "");
      if (parts.size() == 2 && parts[0].containsOnly("0123456789") &&
          parts[1].containsOnly("0123456789")) {
        isInternalArt = true;
      }
    }

    if (isInternalArt) {
      if (sourceArchive.existsAsFile()) {
        data = extractResource(sourceArchive, metadata.artworkPath);
      }
    } else {
      juce::File artFile(metadata.artworkPath);
      if (artFile.existsAsFile()) {
        artFile.loadFileAsData(data);
      }
    }

    if (data.getSize() > 0) {
      size_t offset = payload.getSize();
      size_t size = data.getSize();
      payload.append(data.getData(), data.getSize());
      metadata.artworkPath = juce::String(offset) + ":" +
                             juce::String(size); // Store as offset:size
    }
  }

  // Handle Samples
  for (auto &mapping : metadata.mappings) {
    if (!mapping.samplePath.isEmpty()) {
      juce::MemoryBlock data;

      bool isInternal = false;
      if (mapping.samplePath.contains(":")) {
        auto parts = juce::StringArray::fromTokens(mapping.samplePath, ":", "");
        if (parts.size() == 2 && parts[0].containsOnly("0123456789") &&
            parts[1].containsOnly("0123456789")) {
          isInternal = true;
        }
      }

      if (isInternal) {
        if (sourceArchive.existsAsFile()) {
          data = extractResource(sourceArchive, mapping.samplePath);
        }
      } else {
        juce::File sFile(mapping.samplePath);
        if (sFile.existsAsFile()) {
          sFile.loadFileAsData(data);
        }
      }

      if (data.getSize() > 0) {
        // Ensure 8-byte alignment for ARM performance
        while (payload.getSize() % 8 != 0) {
          const char zero = 0;
          payload.append(&zero, 1);
        }

        size_t offset = payload.getSize();
        size_t size = data.getSize();
        payload.append(data.getData(), data.getSize());

        // Internal reference: "OFFSET:SIZE"
        mapping.samplePath = juce::String(offset) + ":" + juce::String(size);
      } else {
        // failed to locate asset
        mapping.samplePath = "";
      }
    }
  }

  // Now write the file securely (after reading from source archive is done in
  // case they are the same file)
  juce::FileOutputStream stream(outputFile);
  if (!stream.openedOk())
    return false;

  stream.setPosition(0);
  stream.truncate();
  // 2. Serialize Metadata with internal offsets
  juce::String xmlString = SoteroMetadataHandler::toXmlString(metadata);
  juce::MemoryBlock xmlData;

  if (!metadata.dna.isEmpty()) {
    // Obfuscate (Protect)
    xmlData = SoteroSecurity::obfuscate(xmlString, "SOTERO_SHIELD_V1");
  } else {
    xmlData.append(xmlString.toRawUTF8(), xmlString.getNumBytesAsUTF8());
  }

  // 3. Write Header
  SoteroHeader header;
  memcpy(header.magic, constants::kMagicNumber, 8);
  header.version = constants::kCurrentVersion;
  header.xmlMetadataSize = (uint32_t)xmlData.getSize();
  header.encryptionFlags = metadata.dna.isEmpty() ? 0 : 1; // 1 = Protected

  stream.write(&header, sizeof(SoteroHeader));

  // 4. Write XML Manifest
  if (xmlData.getSize() > 0)
    stream.write(xmlData.getData(), xmlData.getSize());

  // 5. Write Binary Payload
  if (payload.getSize() > 0)
    stream.write(payload.getData(), payload.getSize());

  stream.flush();
  return true;
}

/**
 * @brief Extracts a binary resource from an archive using an existing stream.
 */
static juce::MemoryBlock extractResource(juce::InputStream &stream,
                                         const juce::String &internalPath) {
  auto parts = juce::StringArray::fromTokens(internalPath, ":", "");
  if (parts.size() < 2)
    return {};

  size_t offset = (size_t)parts[0].getLargeIntValue();
  size_t size = (size_t)parts[1].getLargeIntValue();

  stream.setPosition(0);
  SoteroHeader header;
  if (stream.read(&header, sizeof(SoteroHeader)) < sizeof(SoteroHeader))
    return {};

  // Byte offset in file = Header + XML + internal offset
  int64_t absoluteOffset =
      sizeof(SoteroHeader) + header.xmlMetadataSize + offset;
  stream.setPosition(absoluteOffset);

  juce::MemoryBlock data;
  stream.readIntoMemoryBlock(data, (int)size);
  return data;
}

/**
 * @brief Extracts a binary resource from an archive.
 */
static juce::MemoryBlock extractResource(const juce::File &archiveFile,
                                         const juce::String &internalPath) {
  juce::FileInputStream stream(archiveFile);
  if (!stream.openedOk())
    return {};

  return extractResource(stream, internalPath);
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

  juce::String xmlString;
  if (header.encryptionFlags & 1) {
    xmlString = SoteroSecurity::deobfuscate(xmlData, "SOTERO_SHIELD_V1");
  } else {
    xmlString = xmlData.toString();
  }
  return SoteroMetadataHandler::fromXmlString(xmlString);
}
};
} // namespace sotero
