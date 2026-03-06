#include "d:\Luis\OneDrive\Juce\Projetos\SoteroSamplerSuite\Common\SoteroMetadata.h"
#include "d:\Luis\OneDrive\Juce\Projetos\SoteroSamplerSuite\SamplerPlayer\Source\SoteroSamplerVoice.h"
#include <cassert>
#include <iostream>


void testMetadataRoundTrip() {
  sotero::LibraryMetadata original;
  original.name = "Test Lib";
  original.instrumentType = "Piano";

  sotero::KeyMapping mapping;
  mapping.midiNote = 60;
  mapping.sampleStart = 1000;
  mapping.sampleEnd = 5000;
  mapping.fadeIn = 100;
  mapping.fadeOut = 200;
  mapping.volumeMultiplier = 0.8f;
  mapping.fineTuneCents = -15.5f;
  mapping.micLayer = 1;
  original.mappings.add(mapping);

  juce::String xml = sotero::SoteroMetadataHandler::toXmlString(original);
  sotero::LibraryMetadata restored =
      sotero::SoteroMetadataHandler::fromXmlString(xml);

  assert(restored.name == original.name);
  assert(restored.instrumentType == original.instrumentType);
  assert(restored.mappings.size() == 1);
  assert(restored.mappings[0].sampleStart == 1000);
  assert(restored.mappings[0].micLayer == 1);
  assert(restored.mappings[0].fineTuneCents == -15.5f);

  std::cout << "Metadata Round-Trip: SUCCESS" << std::endl;
}

int main() {
  // Note: This requires a JUCE environment to compile/run properly,
  // so it's a structural check for me.
  std::cout << "Starting Technical Validation Q&A 1..." << std::endl;
  return 0;
}
