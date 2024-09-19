#include <JuceHeader.h>
#include "MainComponent.h"

//==============================================================================
class iLumidiApplication : public juce::JUCEApplication, public juce::MenuBarModel
{
public:
    //==============================================================================
    iLumidiApplication() = default;

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const juce::String& commandLine) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
        mainWindow->initialize();
        mainWindow->setMenuBar(this);
    }

    void shutdown() override
    {
        mainWindow->setMenuBar(nullptr);
        mainWindow = nullptr;
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
    }

    //==============================================================================
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name,
                juce::Desktop::getInstance().getDefaultLookAndFeel()
                .findColour(juce::ResizableWindow::backgroundColourId),
                DocumentWindow::allButtons),
              mainComponent(new MainComponent())
        {
            setUsingNativeTitleBar(true);
            setContentOwned(mainComponent, true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
#else
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
#endif

            setVisible(true);
            toFront(true);
        }

        void initialize()
        {
            mainComponent->initialize();
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        MainComponent* getMainComponent() const { return mainComponent; }

    private:
        MainComponent* mainComponent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;

    // Menu bar methods
    juce::StringArray getMenuBarNames() override
    {
        return { "File", "Edit", "View", "Help" };
    }

    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override
    {
        juce::PopupMenu menu;

        if (menuName == "File")
        {
            menu.addItem("Settings", [this] {
                if (auto* mainComponent = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
                    mainComponent->showSettingsWindow();
            });
            menu.addSeparator();
            menu.addItem("Quit", [] { JUCEApplication::getInstance()->systemRequestedQuit(); });
        }

        // Add other menu items for "Edit", "View", and "Help" as needed

        return menu;
    }

    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override
    {
        // Handle menu item selection if needed
    }
};

//==============================================================================
START_JUCE_APPLICATION(iLumidiApplication)