#include "../../Common/SoteroViews.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

// SamplerPlayer specific editor
class SamplerPlayerAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  SamplerPlayerAudioProcessorEditor(SamplerPlayerAudioProcessor &);
  ~SamplerPlayerAudioProcessorEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  SamplerPlayerAudioProcessor &audioProcessor;
  std::unique_ptr<sotero::SoteroPlayerUI> playerUI;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      SamplerPlayerAudioProcessorEditor)
};
