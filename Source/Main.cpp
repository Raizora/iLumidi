#include <JuceHeader.h>
#include "MainComponent.h"

//==============================================================================
// Defines the iLumidiApplication class which inherits from JUCEApplication and MenuBarModel
class iLumidiApplication : public juce::JUCEApplication, public juce::MenuBarModel
{
public:
    //==============================================================================
    iLumidiApplication() = default; // Default constructor

    // Returns the application name
    const juce::String getApplicationName() override { return ProjectInfo::projectName; }

    // Returns the application version
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }

    // Allows more than one instance of the application
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    // Method to initialize the application
    void initialise(const juce::String& commandLine) override
    {
        // This method is where you should put your application's initialisation code..

        mainWindow = std::make_unique<MainWindow>(getApplicationName()); // Creates the main window
        mainWindow->initialize(); // Initialize the main window and its components after construction
        mainWindow->setMenuBar(this); // Sets the menu bar for the main window
    }

    // Method to shutdown the application
    void shutdown() override
    {
        // Add your application's shutdown code here..

        mainWindow->setMenuBar(nullptr); // Removes the menu bar
        mainWindow = nullptr; // (deletes our window) // Deletes the main window
    }

    //==============================================================================
    // Called when the system requests the application to quit
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit(); // Quits the application
    }

    // Called when another instance of the application is started
    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        // When another instance of the app is launched while this one is running, this
        // method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

    //==============================================================================
    // Implements the desktop window that contains an instance of our MainComponent class.
    class MainWindow : public juce::DocumentWindow
    {
    public:
        // Constructor for the MainWindow class
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name,
                juce::Desktop::getInstance().getDefaultLookAndFeel()
                .findColour(juce::ResizableWindow::backgroundColourId),
                DocumentWindow::allButtons),
              mainComponent(new MainComponent()) // Initialize mainComponent here
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

        // Method to initialize components after construction
        void initialize()
        {
            mainComponent->initialize(); // Initialize the main component after construction
        }

        // Called when the close button is pressed
        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit(); // Request the application to quit
        }

        MainComponent* getMainComponent() const { return mainComponent; } // Getter for the main component

    private:
        MainComponent* mainComponent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow) // Prevents copying of the MainWindow class
    };

private:
    std::unique_ptr<MainWindow> mainWindow; // Pointer to the main window

    // Returns the names of the menu bar items
    juce::StringArray getMenuBarNames() override
    {
        return { "File", "Edit", "View", "Help" }; // Define the menu bar items
    }

    // Returns the popup menu for a given index
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override
    {
        juce::PopupMenu menu;

        if (menuName == "File")
        {
            menu.addItem("Settings", [this] { // Add "Settings" menu item with a lambda function
                if (auto* mainComponent = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
                    mainComponent->showSettingsWindow(); // Show settings window when "Settings" is clicked
            });
            menu.addSeparator(); // Add a separator
            menu.addItem("Quit", [] { JUCEApplication::getInstance()->systemRequestedQuit(); }); // Add "Quit" menu item with a lambda function
        }

        // Add other menu items for "Edit", "View", and "Help" as needed

        return menu; // Return the constructed menu
    }

    // Handles menu item selection
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override
    {
        // Handle menu item selection if needed
    }
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(iLumidiApplication) // Macro to start the JUCE application