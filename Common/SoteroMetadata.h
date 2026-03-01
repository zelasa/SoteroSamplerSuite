#pragma once

#include "SoteroFormat.h"
#include <juce_core/juce_core.h>
#include <memory>

namespace sotero {
/**
 * @class SoteroMetadataHandler
 * @brief Helper class to convert LibraryMetadata to/from XML.
 */
class SoteroMetadataHandler {
public:
  /**
   * @brief Converts LibraryMetadata to an XML string.
   */
  static juce::String toXmlString(const LibraryMetadata &metadata) {
    auto xml = std::make_unique<juce::XmlElement>("SoteroLibrary");
    xml->setAttribute("version", "1.0");
    xml->setAttribute("name", metadata.name);
    xml->setAttribute("author", metadata.author);
    xml->setAttribute("description", metadata.description);

    auto artwork = xml->createNewChildElement("Artwork");
    artwork->setAttribute("path", metadata.artworkPath);

    auto mappings = xml->createNewChildElement("Mappings");
    for (const auto &mapping : metadata.mappings) {
      auto m = mappings->createNewChildElement("Mapping");
      m->setAttribute("note", mapping.midiNote);
      m->setAttribute("sample", mapping.samplePath);
      m->setAttribute("fileName", mapping.fileName);
      m->setAttribute("velLow", mapping.velocityLow);
      m->setAttribute("velHigh", mapping.velocityHigh);
      m->setAttribute("choke", mapping.chokeGroup);
    }

    return xml->toString(juce::XmlElement::TextFormat().withoutHeader());
  }

  /**
   * @brief Parses an XML string into a LibraryMetadata structure.
   */
  static LibraryMetadata fromXmlString(const juce::String &xmlString) {
    LibraryMetadata metadata;
    auto xml = juce::XmlDocument::parse(xmlString);

    if (xml != nullptr && xml->hasTagName("SoteroLibrary")) {
      metadata.name = xml->getStringAttribute("name");
      metadata.author = xml->getStringAttribute("author");
      metadata.description = xml->getStringAttribute("description");

      if (auto *artwork = xml->getChildByName("Artwork"))
        metadata.artworkPath = artwork->getStringAttribute("path");

      if (auto *mappings = xml->getChildByName("Mappings")) {
        for (auto *m : mappings->getChildIterator()) {
          KeyMapping mapping;
          mapping.midiNote = m->getIntAttribute("note");
          mapping.samplePath = m->getStringAttribute("sample");
          mapping.fileName = m->getStringAttribute("fileName");
          mapping.velocityLow = m->getIntAttribute("velLow");
          mapping.velocityHigh = m->getIntAttribute("velHigh");
          mapping.chokeGroup = m->getIntAttribute("choke");
          metadata.mappings.add(mapping);
        }
      }
    }

    return metadata;
  }
};
} // namespace sotero
