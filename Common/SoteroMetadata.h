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
    xml->setAttribute("creationDate", metadata.creationDate);
    xml->setAttribute("instrumentType", metadata.instrumentType);

    // Toggles
    auto toggles = xml->createNewChildElement("Toggles");
    toggles->setAttribute("compressor", metadata.enableCompressor);
    toggles->setAttribute("eq", metadata.enableEQ);
    toggles->setAttribute("reverb", metadata.enableReverb);
    toggles->setAttribute("punch", metadata.enablePunch);
    toggles->setAttribute("dna", metadata.dna); // Added dna property
    toggles->setAttribute("adsr", metadata.enableADSR);
    toggles->setAttribute("filter", metadata.enableFilter);
    toggles->setAttribute("loopCancellation", metadata.loopCancellationMode);

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

      // Non-destructive
      m->setAttribute("start", (int)mapping.sampleStart);
      m->setAttribute("end", (int)mapping.sampleEnd);
      m->setAttribute("fadeIn", (int)mapping.fadeIn);
      m->setAttribute("fadeOut", (int)mapping.fadeOut);
      m->setAttribute("volume", (double)mapping.volumeMultiplier);
      m->setAttribute("fineTune", (double)mapping.fineTuneCents);
      m->setAttribute("micLayer", mapping.micLayer);

      // Sculpting
      m->setAttribute("adsrA", (double)mapping.adsrAttack);
      m->setAttribute("adsrD", (double)mapping.adsrDecay);
      m->setAttribute("adsrS", (double)mapping.adsrSustain);
      m->setAttribute("adsrR", (double)mapping.adsrRelease);
      m->setAttribute("fType", mapping.filterType);
      m->setAttribute("fCut", (double)mapping.filterCutoff);
      m->setAttribute("fRes", (double)mapping.filterResonance);
    }

    auto loops = xml->createNewChildElement("Loops");
    for (const auto &loop : metadata.loops) {
      auto l = loops->createNewChildElement("Loop");
      l->setAttribute("slot", loop.slotIndex);
      l->setAttribute("path", loop.midiPath);
      l->setAttribute("name", loop.name);
      l->setAttribute("sync", loop.syncToHost);
      l->setAttribute("tempoMult", (double)loop.tempoMultiplier);
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
      metadata.creationDate = xml->getStringAttribute("creationDate");
      metadata.instrumentType = xml->getStringAttribute("instrumentType");

      if (auto *toggles = xml->getChildByName("Toggles")) {
        metadata.enableCompressor = toggles->getBoolAttribute("compressor");
        metadata.enableEQ = toggles->getBoolAttribute("eq");
        metadata.enableReverb = toggles->getBoolAttribute("reverb");
        metadata.enablePunch = toggles->getBoolAttribute("punch");
        metadata.dna = toggles->getStringAttribute("dna"); // Added dna property
        metadata.enableADSR = toggles->getBoolAttribute("adsr", true);
        metadata.enableFilter = toggles->getBoolAttribute("filter", true);
        metadata.loopCancellationMode =
            toggles->getBoolAttribute("loopCancellation");
      }

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

          mapping.sampleStart = m->getIntAttribute("start");
          mapping.sampleEnd = m->getIntAttribute("end");
          mapping.fadeIn = m->getIntAttribute("fadeIn");
          mapping.fadeOut = m->getIntAttribute("fadeOut");
          mapping.volumeMultiplier =
              (float)m->getDoubleAttribute("volume", 1.0);
          mapping.fineTuneCents = (float)m->getDoubleAttribute("fineTune", 0.0);
          mapping.micLayer = m->getIntAttribute("micLayer", 0);

          // Sculpting
          mapping.adsrAttack = (float)m->getDoubleAttribute("adsrA", 0.01);
          mapping.adsrDecay = (float)m->getDoubleAttribute("adsrD", 0.1);
          mapping.adsrSustain = (float)m->getDoubleAttribute("adsrS", 1.0);
          mapping.adsrRelease = (float)m->getDoubleAttribute("adsrR", 0.1);
          mapping.filterType = m->getIntAttribute("fType", 0);
          mapping.filterCutoff = (float)m->getDoubleAttribute("fCut", 20000.0);
          mapping.filterResonance = (float)m->getDoubleAttribute("fRes", 1.0);

          metadata.mappings.add(mapping);
        }
      }

      if (auto *loops = xml->getChildByName("Loops")) {
        for (auto *l : loops->getChildIterator()) {
          LoopMapping loop;
          loop.slotIndex = l->getIntAttribute("slot");
          loop.midiPath = l->getStringAttribute("path");
          loop.name = l->getStringAttribute("name");
          loop.syncToHost = l->getBoolAttribute("sync", true);
          loop.tempoMultiplier = (float)l->getDoubleAttribute("tempoMult", 1.0);
          metadata.loops.add(loop);
        }
      }
    }

    return metadata;
  }
};
} // namespace sotero
