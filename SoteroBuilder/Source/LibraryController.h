#pragma once

#include "../../Common/SoteroFormat.h"
#include "../../Common/SoteroArchive.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_events/juce_events.h>

namespace sotero {

/**
 * @class LibraryController
 * @brief Central authority for managing LibraryMetadata.
 * 
 * This class decouples the data model from the UI components.
 * It provides a thread-safe way to modify the library and notifies 
 * listeners when changes occur.
 */
class LibraryController : public juce::ChangeBroadcaster {
public:
    LibraryController();
    ~LibraryController() override = default;

    // --- Data Access ---
    const LibraryMetadata& getMetadata() const { return metadata; }
    
    // --- Library Actions ---
    void createNew();
    void loadFromFile(const juce::File& file);
    void saveToFile(const juce::File& file, const juce::File& sourceResourcesDir);

    // --- Mapping Management ---
    void addMapping(const KeyMapping& mapping);
    void updateMapping(int index, const KeyMapping& newMapping);
    void removeMapping(int index);
    void clearMappings();

    // --- Global Metadata ---
    void setLibraryName(const juce::String& name);
    void setAuthor(const juce::String& author);
    void setDescription(const juce::String& desc);
    void setArtwork(const juce::Image& image);
    juce::Image getArtwork() const { return artwork; }

    // --- State Queries ---
    bool hasUnsavedChanges() const { return unsavedChanges; }
    juce::File getCurrentFile() const { return currentFile; }

private:
    LibraryMetadata metadata;
    juce::Image artwork;
    juce::File currentFile;
    bool unsavedChanges = false;

    void notifyListeners();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryController)
};

} // namespace sotero
