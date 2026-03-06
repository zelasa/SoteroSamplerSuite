#pragma once

#include "SoteroArchive.h"
#include "SoteroMetadata.h"
#include "SoteroSecurity.h"
#include <juce_core/juce_core.h>

namespace sotero {

/**
 * @class SoteroLibraryManager
 * @brief Manages the scanning, listing, and metadata of available Sotero
 * Libraries.
 */
class SoteroLibraryManager {
public:
  struct LibraryEntry {
    juce::String name;
    juce::String author;
    juce::File file;
    juce::String instrumentType;
    bool isLocked = false;
  };

  SoteroLibraryManager() { refresh(); }

  /**
   * @brief Scans the default library directory.
   * Default: Documents/Sotero/Libraries
   */
  void refresh() {
    entries.clear();
    auto libDir = getDefaultLibraryDirectory();

    if (!libDir.exists())
      libDir.createDirectory();

    juce::Array<juce::File> files;
    libDir.findChildFiles(files, juce::File::findFiles, false,
                          "*.spsa;*.sotero");

    for (auto &file : files) {
      auto meta = SoteroArchive::readMetadata(file);
      LibraryEntry entry;
      entry.name =
          meta.name.isEmpty() ? file.getFileNameWithoutExtension() : meta.name;
      entry.author = meta.author;
      entry.file = file;
      entry.instrumentType = meta.instrumentType;
      entry.isLocked = !SoteroSecurity::validateDNA(meta.dna);
      entries.add(entry);
    }
  }

  const juce::Array<LibraryEntry> &getLibraries() const { return entries; }

  static juce::File getDefaultLibraryDirectory() {
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Sotero")
        .getChildFile("Libraries");
  }

private:
  juce::Array<LibraryEntry> entries;
};

} // namespace sotero
