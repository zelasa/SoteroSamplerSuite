#include "SoteroLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace sotero {

/**
 * @class MetadataWidget
 * @brief Standalone widget for library metadata (Name, Author, Date, Artwork).
 */
class MetadataWidget : public juce::Component {
public:
    MetadataWidget() {
        addAndMakeVisible(nameEditor);
        addAndMakeVisible(authorEditor);
        addAndMakeVisible(dateEditor);
        addAndMakeVisible(infoEditor);
        addAndMakeVisible(nameLabel);
        addAndMakeVisible(authorLabel);
        addAndMakeVisible(dateLabel);
        addAndMakeVisible(infoLabel);
        addAndMakeVisible(artworkDrop);
        addAndMakeVisible(artworkLabel);
        addAndMakeVisible(volSlider);
        addAndMakeVisible(volLabel);

        volLabel.setText("MASTER VOL", juce::dontSendNotification);
        volLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
        volLabel.setFont(12.0f);

        volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        volSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        volSlider.setTextValueSuffix(" dB");

        nameLabel.setText("LIBRARY NAME", juce::dontSendNotification);
        nameLabel.setColour(juce::Label::textColourId, SoteroLookAndFeel::getLayer1Colour());
        authorLabel.setText("AUTHOR", juce::dontSendNotification);
        authorLabel.setColour(juce::Label::textColourId, SoteroLookAndFeel::getLayer1Colour());

        artworkLabel.setJustificationType(juce::Justification::centred);
        artworkLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        
        // Editors will now use global LookAndFeel defaults
    }

    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);
        
        // Draw VUs (simplified for now, keep logic in widget)
        g.setColour(juce::Colours::black);
        g.fillRect(vuL);
        g.fillRect(vuR);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(5);

        auto artworkArea = r.removeFromTop(120);
        auto artSquare = artworkArea.removeFromLeft(120);
        artworkDrop.setBounds(artSquare);
        artworkLabel.setBounds(artSquare);

        auto fields = artworkArea.reduced(10, 0);
        nameLabel.setBounds(fields.removeFromTop(20));
        nameEditor.setBounds(fields.removeFromTop(25));
        fields.removeFromTop(5);
        authorLabel.setBounds(fields.removeFromTop(20));
        authorEditor.setBounds(fields.removeFromTop(30));

        // Adjust VU positions
        auto vuArea = r.removeFromTop(20).reduced(20, 0);
        vuL = vuArea.removeFromTop(8);
        vuArea.removeFromTop(4);
        vuR = vuArea;

        auto volArea = r.removeFromBottom(40).reduced(10, 0);
        volLabel.setBounds(volArea.removeFromLeft(80));
        volSlider.setBounds(volArea);
    }

    // Accessors for wiring
    juce::TextEditor& getNameEditor() { return nameEditor; }
    juce::TextEditor& getAuthorEditor() { return authorEditor; }
    juce::Slider& getVolSlider() { return volSlider; }

private:
    juce::TextEditor nameEditor, authorEditor, dateEditor, infoEditor;
    juce::Label nameLabel, authorLabel, dateLabel, infoLabel;
    juce::ImageComponent artworkDrop;
    juce::Label artworkLabel{"ArtLabel", "IMAGE DROP"};
    juce::Slider volSlider;
    juce::Label volLabel;
    juce::Rectangle<int> vuL, vuR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetadataWidget)
};

} // namespace sotero
