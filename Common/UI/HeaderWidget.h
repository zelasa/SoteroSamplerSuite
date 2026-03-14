#include "SoteroLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace sotero {

/**
 * @class HeaderWidget
 * @brief Standalone header component for Sotero applications.
 * Handles New/Load/Save/Export actions and Mode switching.
 */
class HeaderWidget : public juce::Component {
public:
    HeaderWidget() {
        addAndMakeVisible(titleLabel);
        addAndMakeVisible(devModeBtn);
        addAndMakeVisible(userModeBtn);
        addAndMakeVisible(saveBtn);
        addAndMakeVisible(loadBtn);
        addAndMakeVisible(newBtn);
        addAndMakeVisible(closeBtn);
        addAndMakeVisible(toPlayerToggle);
        addAndMakeVisible(versionLabel);

        titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
        titleLabel.setJustificationType(juce::Justification::centred);
        titleLabel.setColour(juce::Label::textColourId, SoteroLookAndFeel::getYellowAccent());

        versionLabel.setJustificationType(juce::Justification::centredLeft);
        versionLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        toPlayerToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::grey);

        // Buttons will now use the LookAndFeel drawButtonBackground
    }

    void resized() override {
        auto r = getLocalBounds().reduced(5);

        // Upper Right: Mode and Project
        auto rightArea = r.removeFromRight(350);
        auto topRow = rightArea.removeFromTop(25);
        devModeBtn.setBounds(topRow.removeFromLeft(125).reduced(2));
        userModeBtn.setBounds(topRow.reduced(2));

        auto bottomRow = rightArea.reduced(0, 2);
        float btnW = bottomRow.getWidth() / 4.0f;
        saveBtn.setBounds(bottomRow.removeFromLeft(btnW).reduced(2));
        loadBtn.setBounds(bottomRow.removeFromLeft(btnW).reduced(2));
        newBtn.setBounds(bottomRow.removeFromLeft(btnW).reduced(2));
        closeBtn.setBounds(bottomRow.reduced(2));

        // Center-ish: Title
        titleLabel.setBounds(r.removeFromTop(30));

        // Version
        versionLabel.setBounds(r.removeFromLeft(100).reduced(2));

        // Toggle
        toPlayerToggle.setBounds(r.removeFromRight(100).reduced(2));
    }

    // Public access to components for wiring (or use callbacks)
    juce::TextButton& getDevModeBtn() { return devModeBtn; }
    juce::TextButton& getUserModeBtn() { return userModeBtn; }
    juce::TextButton& getSaveBtn() { return saveBtn; }
    juce::TextButton& getLoadBtn() { return loadBtn; }
    juce::TextButton& getNewBtn() { return newBtn; }
    juce::TextButton& getCloseBtn() { return closeBtn; }
    juce::ToggleButton& getToPlayerToggle() { return toPlayerToggle; }

    void setTitle(const juce::String& text) { titleLabel.setText(text, juce::dontSendNotification); }
    void setVersion(const juce::String& text) { versionLabel.setText(text, juce::dontSendNotification); }

private:
    juce::Label titleLabel{"Title", "SOTEROPOLYSAMPLES"};
    juce::TextButton devModeBtn{"DEVELOPER"}, userModeBtn{"USER PLAYER"};
    juce::TextButton saveBtn{"SAVE"}, loadBtn{"LOAD"}, newBtn{"NEW"}, closeBtn{"CLOSE"};
    juce::ToggleButton toPlayerToggle{"TO PLAYER"};
    juce::Label versionLabel{"Version", "v0.4.0"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderWidget)
};

} // namespace sotero
