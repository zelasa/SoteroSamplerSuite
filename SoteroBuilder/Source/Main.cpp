#include <JuceHeader.h>

class SoteroBuilderApp : public juce::JUCEApplication {
public:
  SoteroBuilderApp() {}
  const juce::String getApplicationName() override { return "SoteroBuilder"; }
  const juce::String getApplicationVersion() override { return "0.1.0"; }
  bool moreThanOneInstanceAllowed() override { return true; }
  void initialise(const juce::String &commandLine) override {
    mainWindow = std::make_unique<MainWindow>(getApplicationName());
  }
  void shutdown() override { mainWindow = nullptr; }
  void systemRequestedQuit() override { quit(); }
  void anotherInstanceStarted(const juce::String &commandLine) override {}

  class MainWindow : public juce::DocumentWindow {
  public:
    MainWindow(juce::String name)
        : DocumentWindow(name, juce::Colours::darkgrey,
                         DocumentWindow::allButtons) {
      setUsingNativeTitleBar(true);
      setResizable(true, true);
      centreWithSize(600, 400);
      setVisible(true);
    }
    void closeButtonPressed() override {
      JUCEApplication::getInstance()->systemRequestedQuit();
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
  };

private:
  std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(SoteroBuilderApp)
