#pragma once

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
namespace sotero {

/**
 * @class SoteroSecurity
 * @brief Handles library encryption, HWID validation, and DNA binding
 * foundation.
 */
class SoteroSecurity {
public:
  /**
   * @brief Generates a unique Machine ID for DNA binding.
   */
  static juce::String getMachineID() {
    auto raw = juce::SystemStats::getLogonName() + "@" +
               juce::SystemStats::getDeviceDescription() + ":" +
               juce::SystemStats::getOperatingSystemName();

    juce::MD5 hash(raw.toUTF8());
    return hash.toHexString();
  }

  /**
   * @brief Simple XOR-based obfuscation for metadata XML.
   * key: A dynamic key (MachineID + LibraryID)
   */
  static juce::MemoryBlock obfuscate(const juce::String &xml,
                                     const juce::String &key) {
    juce::MemoryBlock data;
    auto utf8 = xml.toRawUTF8();
    auto size = (size_t)xml.getNumBytesAsUTF8();
    data.append(utf8, size);

    for (size_t i = 0; i < data.getSize(); ++i) {
      data[i] ^= (juce::uint8)key[i % key.length()];
    }
    return data;
  }

  /**
   * @brief Simple XOR-based de-obfuscation.
   */
  static juce::String deobfuscate(const juce::MemoryBlock &data,
                                  const juce::String &key) {
    juce::MemoryBlock result(data);
    for (size_t i = 0; i < result.getSize(); ++i) {
      result[i] ^= (juce::uint8)key[i % key.length()];
    }
    return result.toString();
  }

  /**
   * @brief Validates if a library's DNA matches the current machine.
   * (Foundation for serial verification)
   */
  static bool validateDNA(const juce::String &libraryMachineID) {
    if (libraryMachineID.isEmpty() || libraryMachineID == "DEV")
      return true; // Bypass for development

    return libraryMachineID == getMachineID();
  }
};

} // namespace sotero
