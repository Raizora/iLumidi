/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

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
        // This method is where you should put your application's initialisation code..

        mainWindow = std::make_unique<MainWindow>(getApplicationName());
        mainWindow->setMenuBar(this);
    }

    void shutdown() override
    {
        // Add your application's shutdown code here..

        mainWindow->setMenuBar(nullptr);
        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        /*
            This is called when the app is being asked to quit: you can ignore this
            request and let the app carry on running, or call quit() to allow the app to close.
        */
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        /*
            When another instance of the app is launched while this one is running, Add
            this method is invoked, and the commandLine parameter tells you what
            the other instance's command-line arguments were.
        */
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name,
                juce::Desktop::getInstance().getDefaultLookAndFeel()
                .findColour(juce::ResizableWindow::backgroundColourId),
                DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
#else
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
#endif

            setVisible(true);
            toFront(true);  // Ensure the window is brought to the front
        }

        void closeButtonPressed() override
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;

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
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(iLumidiApplication)
