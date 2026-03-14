#include "LibraryController.h"

namespace sotero {

LibraryController::LibraryController() {
    createNew();
}

void LibraryController::createNew() {
    metadata = LibraryMetadata();
    metadata.mappings.clear();
    metadata.loops.clear();
    artwork = juce::Image();
    currentFile = juce::File();
    unsavedChanges = false;
    notifyListeners();
}

void LibraryController::loadFromFile(const juce::File& file) {
    // Note: Implementation of loading will need to reconcile with SoteroArchive
    // For now, we'll assume a pattern where we update our internal metadata.
    // This will likely be refined when we integrate with SoteroArchive::read.
    currentFile = file;
    unsavedChanges = false;
    notifyListeners();
}

void LibraryController::saveToFile(const juce::File& file, const juce::File& sourceResourcesDir) {
    if (sotero::SoteroArchive::write(file, metadata, sourceResourcesDir)) {
        currentFile = file;
        unsavedChanges = false;
        notifyListeners();
    }
}

void LibraryController::addMapping(const KeyMapping& mapping) {
    metadata.mappings.add(mapping);
    unsavedChanges = true;
    notifyListeners();
}

void LibraryController::updateMapping(int index, const KeyMapping& newMapping) {
    if (index >= 0 && index < metadata.mappings.size()) {
        metadata.mappings.set(index, newMapping);
        unsavedChanges = true;
        notifyListeners();
    }
}

void LibraryController::removeMapping(int index) {
    if (index >= 0 && index < metadata.mappings.size()) {
        metadata.mappings.remove(index);
        unsavedChanges = true;
        notifyListeners();
    }
}

void LibraryController::clearMappings() {
    metadata.mappings.clear();
    unsavedChanges = true;
    notifyListeners();
}

void LibraryController::setLibraryName(const juce::String& name) {
    if (metadata.name != name) {
        metadata.name = name;
        unsavedChanges = true;
        notifyListeners();
    }
}

void LibraryController::setAuthor(const juce::String& author) {
    if (metadata.author != author) {
        metadata.author = author;
        unsavedChanges = true;
        notifyListeners();
    }
}

void LibraryController::setDescription(const juce::String& desc) {
    if (metadata.description != desc) {
        metadata.description = desc;
        unsavedChanges = true;
        notifyListeners();
    }
}

void LibraryController::setArtwork(const juce::Image& image) {
    artwork = image;
    unsavedChanges = true;
    notifyListeners();
}

void LibraryController::notifyListeners() {
    sendChangeMessage();
}

} // namespace sotero
