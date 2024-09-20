#include "MainComponent.h"
#include "CustomLookAndFeel.h"
#include <algorithm>
#include <vector>
#include <ranges>

//==============================================================================
class MainComponent::SettingsWindowCloseButtonHandler : public juce::DocumentWindow
{
public:
    SettingsWindowCloseButtonHandler(const juce::String& name, juce::Colour backgroundColour, int buttons, MainComponent* owner)
        : DocumentWindow(name, backgroundColour, buttons), ownerComponent(owner)
    {
    }

    void closeButtonPressed() override
    {
        if (ownerComponent != nullptr)
        {
            DBG("SettingsWindowCloseButtonHandler: Setting settingsWindow to nullptr");
            ownerComponent->settingsWindow = nullptr;
        }
        setVisible(false);
    }

private:
    MainComponent* ownerComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsWindowCloseButtonHandler)
};

//==============================================================================
MainComponent::MainComponent()
    : settingsWindow(nullptr),
      fadeRate(5.0f), noteColor(juce::Colours::white)
{
    setLookAndFeel(&customLookAndFeel);
    setSize(800, 600);

    instantUpdateToggle.setButtonText("Instant Color Update");
    instantUpdateToggle.setToggleState(false, juce::dontSendNotification);
    instantUpdateToggle.addListener(this);

    disableFadeToggle.setButtonText("Disable Fade");
    disableFadeToggle.addListener(this);
}

void MainComponent::initialize()
{
    DBG("MainComponent initialized. Size: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));

    // Enable MIDI debug messages -----------------------------------------------------------------------***********
    debugMidiMessages = false;
    debugMidiProcessing = false;
    enableNoteEventCount = false;
    // --------------------------------------------------------------------------------------------------***********

    refreshMidiInputs();

    DBG("Opening selected MIDI inputs:");
    openSelectedMidiInputs();

    fadeRateSlider.setRange(0.1, 20.0, 0.1);
    fadeRateSlider.setValue(5.0);
    fadeRateSlider.addListener(this);
    fadeRateSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::white);
    fadeRateSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
    fadeRateSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);

    disableFadeToggle.setButtonText("Disable Fade");
    disableFadeToggle.addListener(this);
    addAndMakeVisible(disableFadeToggle);

    fadeRate = static_cast<float>(fadeRateSlider.getValue());

    noteColorSelector.setCurrentColour(juce::Colours::white);
    noteColorSelector.addChangeListener(this);

    noteColor = noteColorSelector.getCurrentColour();

    scanButton.setButtonText("Scan");
    scanButton.addListener(this);
    scanButton.setColour(juce::TextButton::buttonColourId, juce::Colours::lightblue);
    scanButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    scanButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);

    applyButton.setButtonText("Apply");
    applyButton.addListener(this);
    applyButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    applyButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    applyButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);

    instantUpdateToggle.setButtonText("Instant Color Update");
    instantUpdateToggle.setToggleState(false, juce::dontSendNotification);
    instantUpdateToggle.addListener(this);

    // Add buttons to the component
    addAndMakeVisible(scanButton);
    addAndMakeVisible(applyButton);
    addAndMakeVisible(fadeRateSlider);
    addAndMakeVisible(disableFadeToggle);
    addAndMakeVisible(noteColorSelector);
    addAndMakeVisible(instantUpdateToggle);
}

//==============================================================================
MainComponent::~MainComponent()
{
    // **Remove listeners to prevent dangling references**
    fadeRateSlider.removeListener(this);
    disableFadeToggle.removeListener(this);
    scanButton.removeListener(this);
    applyButton.removeListener(this);
    noteColorSelector.removeChangeListener(this);
    instantUpdateToggle.removeListener(this);

    //enableMode1Button.removeListener(this);
    //enableMode2Button.removeListener(this);

    for (auto* deviceToggle : midiDeviceToggles)
        deviceToggle->removeListener(this);

    for (auto* channelToggles : midiChannelToggles)
        for (auto* channelToggle : *channelToggles)
            channelToggle->removeListener(this);

    setLookAndFeel(nullptr);

    for (auto* device : midiInputsOpened)
    {
        if (device != nullptr)
            device->stop();
    }
    midiInputsOpened.clear(true);

    if (settingsWindow != nullptr)
    {
        DBG("MainComponent Destructor: Hiding and resetting settingsWindow");
        settingsWindow->setVisible(false);
        settingsWindow.reset();
    }
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // Fill the background with the look and feel's background color
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    static int paintCallCount = 0;
    static juce::uint32 lastPaintTime = 0;
    juce::uint32 currentTime = juce::Time::getMillisecondCounter();

    // Log paint call details if debugMidiMessages is enabled
    if (debugMidiMessages && (currentTime - lastPaintTime > 1000 || !midiMessages.empty()))
    {
        DBG("Paint called (" + juce::String(++paintCallCount) + "). Number of MIDI messages: " + juce::String(midiMessages.size()) + ", Note events: " + juce::String(noteEventCount));
        lastPaintTime = currentTime;
    }

    g.setColour(juce::Colours::white);
    for (auto& pair : midiMessages)
    {
        auto& message = pair.first;
        float& alpha = pair.second;
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            float velocity = static_cast<float>(message.getVelocity()) / 127.0f;
            float x = static_cast<float>(getWidth()) * static_cast<float>(noteNumber) / 127.0f;
            float height = static_cast<float>(getHeight()) * velocity;

            juce::Colour noteColour = noteColor.withAlpha(alpha);

            g.setColour(noteColour);

            juce::Path triangle;
            triangle.addTriangle(x, static_cast<float>(getHeight()) - height, x + 10.0f, static_cast<float>(getHeight()), x - 10.0f, static_cast<float>(getHeight()));
            g.fillPath(triangle);

            if (!disableFadeToggle.getToggleState())
            {
                alpha *= (1.0f - fadeRate / 100.0f);
                if (debugMidiMessages) {
                    DBG("Fading alpha value; new alpha: " + juce::String(alpha));
                }
            }
        }
    }

    // Remove messages with alpha less than 0.01
    std::erase_if(midiMessages, [](const auto& pair) { return pair.second < 0.01f; });
    if (debugMidiMessages) {
        DBG("Removed messages with alpha < 0.01; remaining count: " + juce::String(midiMessages.size()));
    }
}

//==============================================================================
void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    //enableMode1Button.setBounds(area.removeFromTop(30));
    //enableMode2Button.setBounds(area.removeFromTop(30));
    // Layout other components accordingly
}

//==============================================================================
void MainComponent::noteColorChanged() {
    noteColor = noteColorSelector.getCurrentColour();
    if (instantUpdateToggle.getToggleState()) {
        repaint(); // Apply changes immediately
    }
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    int channel = message.getChannel();

    // Log incoming MIDI message with source and channel information
    if (debugMidiMessages) {
        DBG("Incoming MIDI Message from: " + source->getName() + " Channel: " + juce::String(channel));
    }

    // Log detailed MIDI traffic information if enabled
    if (debugMidiTraffic) {
        juce::String logMessage = "MIDI Message: " + message.getDescription();
        DBG(logMessage); // Log to console

        // Optionally, write to a temporary text file
        juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("midi_traffic_log.txt");
        tempFile.appendText(logMessage + "\n");
    }

    // Only update MIDI device selections if necessary
    static bool midiDeviceSelectionsUpdated = false;
    if (!midiDeviceSelectionsUpdated) {
        updateMidiDeviceSelections();
        midiDeviceSelectionsUpdated = true;
    }

    // Process the MIDI message if it's from a selected device and channel
    if (selectedMidiDevices.contains(source->getName()) &&
        (selectedChannels.isEmpty() || selectedChannels.contains(channel)))
    {
        // Log that the MIDI message is being processed
        if (debugMidiProcessing) {
            DBG("Processing MIDI message");
        }
        processMidiMessage(message);
    }
    else
    {
        // Log that the MIDI message was received from an unselected source
        if (debugMidiProcessing) {
            DBG("MIDI message received from unselected source");
        }
    }
}

//==============================================================================
void MainComponent::processMidiMessage(const juce::MidiMessage& message) {
    midiMessageCount++;
    int channel = message.getChannel(); // Adjust channel number to be 1-based for logging

    // Log the processing of the MIDI message with note number and channel
    if (debugMidiMessages) {
        DBG("Processing MIDI message; Note ON: " + juce::String(message.getNoteNumber()) + ", Channel: " + juce::String(channel));
    }

    if (message.isNoteOn()) {
        midiMessages.emplace_back(message, 1.0f);

        // Increment note event count if enabled
        if (enableNoteEventCount) {
            noteEventCount++;
            if (debugMidiMessages) {
                DBG("Note event count incremented; new count: " + juce::String(noteEventCount));
            }
        }

        // Log the addition of the message to the midiMessages vector
        if (debugMidiMessages) {
            DBG("Added message to midiMessages; count: " + juce::String(midiMessages.size()));
        }

        // Remove the oldest message if the vector size exceeds 100
        if (midiMessages.size() > 100) {
            midiMessages.erase(midiMessages.begin());
            if (debugMidiMessages) {
                DBG("Erased oldest message; new count: " + juce::String(midiMessages.size()));
            }
        }
    }

    // Start the timer to repaint at 60 FPS if it's not already running
    if (!isTimerRunning()) {
        startTimerHz(60);
        if (debugMidiMessages) {
            DBG("Started timer for repainting at 60 FPS");
        }
    }
}

//==============================================================================
void MainComponent::timerCallback()
{
    repaint();

    // Stop the timer if there are no MIDI messages to process
    if (midiMessages.empty())
        stopTimer();
}

//==============================================================================
void MainComponent::refreshMidiInputs()
{
    DBG("Starting refreshMidiInputs()");
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    DBG("Refreshing MIDI Inputs:");

    // Update MIDI devices
    for (const auto& input : midiInputs)
    {
        DBG("Found MIDI input: " + input.name + " (ID: " + input.identifier + ")");

        int existingIndex = midiDevicesList.indexOf(input.name);
        if (existingIndex != -1)
        {
            continue;
        }

        midiDevicesList.add(input.name);

        auto deviceToggle = std::make_unique<juce::ToggleButton>(input.name);
        deviceToggle->setColour(juce::ToggleButton::textColourId, juce::Colours::black);
        deviceToggle->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::black);
        deviceToggle->setColour(juce::ToggleButton::tickColourId, juce::Colours::black);
        deviceToggle->addListener(this);
        midiDeviceToggles.add(deviceToggle.release());

        auto channelToggles = std::make_unique<juce::OwnedArray<juce::ToggleButton>>();
        for (int i = 1; i <= 16; ++i)
        {
            auto channelToggle = std::make_unique<juce::ToggleButton>("Ch " + juce::String(i));
            channelToggle->setColour(juce::ToggleButton::textColourId, juce::Colours::black);
            channelToggle->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::black);
            channelToggle->setColour(juce::ToggleButton::tickColourId, juce::Colours::black);
            channelToggle->addListener(this);
            channelToggles->add(channelToggle.release());
        }
        midiChannelToggles.add(channelToggles.release());
    }

    // Remove devices that are no longer present
    for (int i = midiDevicesList.size() - 1; i >= 0; --i)
    {
        bool deviceStillPresent = false;
        for (const auto& input : midiInputs)
        {
            if (input.name == midiDevicesList[i])
            {
                deviceStillPresent = true;
                break;
            }
        }
        if (!deviceStillPresent)
        {
            midiDevicesList.remove(i);
            midiDeviceToggles.remove(i);
            midiChannelToggles.remove(i);
        }
    }

    updateMidiDeviceSelections();

    DBG("Calling refreshSettingsWindow()");
    juce::MessageManager::callAsync([this] { refreshSettingsWindow(); });

    DBG("Finished refreshMidiInputs()");
}

//==============================================================================
void MainComponent::applyMidiSelections()
{
    DBG("Apply button pushed. Applying MIDI selections.");

    // Close all currently open MIDI inputs
    for (auto* device : midiInputsOpened)
    {
        device->stop();
    }
    midiInputsOpened.clear();

    // Now open the newly selected MIDI inputs
    openSelectedMidiInputs();

    DBG("MIDI selections applied successfully.");
}

//==============================================================================
void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &fadeRateSlider)
    {
        fadeRate = static_cast<float>(fadeRateSlider.getValue());
        // Log the change in fade rate slider value
        DBG("Slider: Fade rate changed to " + juce::String(fadeRate));
    }
}

//==============================================================================
void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &noteColorSelector)
    {
        noteColor = noteColorSelector.getCurrentColour();
        // Log the change in note color
        DBG("ChangeListener: Note color changed to " + noteColor.toString());
    }
}

//==============================================================================
void MainComponent::buttonClicked(juce::Button* button)
{
    // Log the button click event with the button's name
    DBG("Button clicked: " + button->getName());

    // Check if the clicked button is one of our MIDI devices or channel toggles
    bool isMidiToggle = false;
    for (auto* deviceToggle : midiDeviceToggles)
    {
        if (button == deviceToggle)
        {
            isMidiToggle = true;
            break;
        }
    }

    if (!isMidiToggle)
    {
        for (auto* channelToggles : midiChannelToggles)
        {
            for (auto* channelToggle : *channelToggles)
            {
                if (button == channelToggle)
                {
                    isMidiToggle = true;
                    break;
                }
            }
            if (isMidiToggle)
                break;
        }
    }

    if (isMidiToggle)
    {
        // Log the update of MIDI device selections
        DBG("Updating MIDI device selections");
        updateMidiDeviceSelections();
    }
    else if (button == &disableFadeToggle)
    {
        // Log the state change of the disable fade toggle button
        DBG("Disable Fade toggled to " + juce::String(disableFadeToggle.getToggleState() ? "ON" : "OFF"));
        fadeToggleChanged();
    }
    else if (button == &scanButton)
    {
        // Log the click event of the scan button
        DBG("Scan button clicked");
        refreshMidiInputs();
    }
    else if (button == &applyButton)
    {
        // Log the click event of the apply button
        DBG("Apply button clicked");
        applyMidiSelections();
    }
    else if (button == &instantUpdateToggle)
    {
        instantUpdateMode = instantUpdateToggle.getToggleState();
        // Log the state change of the instant update toggle button
        DBG("Instant update mode: " + juce::String(instantUpdateMode ? "true" : "false"));
    }
}

//==============================================================================
void MainComponent::refreshSettingsWindow()
{
    DBG("Starting refreshSettingsWindow()");
    if (settingsWindow == nullptr)
    {
        DBG("Settings window is null, cannot refresh");
        return;
    }

    try
    {
        auto* content = dynamic_cast<juce::Component*>(settingsWindow->getContentComponent());
        if (content == nullptr)
        {
            DBG("Content is null, cannot refresh");
            return;
        }

        DBG("Clearing existing content");
        content->removeAllChildren();
        ownedSettingsComponents.clear();

        int yPos = 10;

        auto addLabel = [&](const juce::String& text) {
            auto label = std::make_unique<juce::Label>();
            label->setText(text, juce::dontSendNotification);
            label->setBounds(10, yPos, 380, 20);
            content->addAndMakeVisible(label.get());
            ownedSettingsComponents.add(label.release());
            yPos += 25;
        };

        // MIDI Devices Section
        addLabel("MIDI Devices:");

        DBG("Refreshing MIDI device and channel toggles");
        for (int i = 0; i < midiDeviceToggles.size(); ++i)
        {
            auto* deviceToggle = midiDeviceToggles[i];
            deviceToggle->setBounds(10, yPos, 380, 20);
            content->addAndMakeVisible(deviceToggle);
            yPos += 25;

            auto* channelToggles = midiChannelToggles[i];
            for (int j = 0; j < channelToggles->size(); ++j)
            {
                auto* channelToggle = (*channelToggles)[j];
                channelToggle->setBounds(30 + (j % 8) * 45, yPos, 40, 20);
                content->addAndMakeVisible(channelToggle);

                if (j % 8 == 7)
                {
                    yPos += 25;
                }
            }
            yPos += 30;
        }

        // Scan and Apply Buttons
        DBG("Adding Scan and Apply buttons");
        scanButton.setBounds(10, yPos, 185, 30);
        content->addAndMakeVisible(scanButton);
        applyButton.setBounds(205, yPos, 185, 30);
        content->addAndMakeVisible(applyButton);
        yPos += 35;

        // Fade Rate Slider
        addLabel("Fade Rate:");
        fadeRateSlider.setBounds(10, yPos, 380, 30);
        content->addAndMakeVisible(fadeRateSlider);
        yPos += 35;

        // Disable Fade Toggle
        DBG("Adding Disable Fade toggle");
        disableFadeToggle.setBounds(10, yPos, 380, 30);
        content->addAndMakeVisible(disableFadeToggle);
        yPos += 35;

        // Note Color Selector
        addLabel("Note Color:");
        noteColorSelector.setBounds(10, yPos, 380, 300);
        content->addAndMakeVisible(noteColorSelector);
        yPos += 305;

        // Instant Update Toggle
        DBG("Adding Instant Update toggle");
        instantUpdateToggle.setBounds(10, yPos, 380, 30);
        content->addAndMakeVisible(instantUpdateToggle);
        yPos += 35;

        DBG("Resizing content");
        content->setSize(400, yPos);
        settingsWindow->setContentComponentSize(400, yPos);

        DBG("Refreshing settings window complete");
    }
    catch (const std::exception& e)
    {
        DBG("Exception in refreshSettingsWindow(): " + juce::String(e.what()));
    }
    catch (...)
    {
        DBG("Unknown exception in refreshSettingsWindow()");
    }
}

//==============================================================================
class SettingsWindow : public juce::DocumentWindow
{
public:
    SettingsWindow(const juce::String& name, juce::Colour backgroundColour, int buttonsNeeded, MainComponent* owner)
        : juce::DocumentWindow(name, backgroundColour, buttonsNeeded), ownerComponent(owner)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
        if (onCloseCallback)
            onCloseCallback();
    }

    void setOnCloseCallback(std::function<void()> callback)
    {
        onCloseCallback = std::move(callback);
    }

private:
    MainComponent* ownerComponent;
    std::function<void()> onCloseCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsWindow)
};

//==============================================================================
void MainComponent::showSettingsWindow()
{
    DBG("Showing settings window");

    if (settingsWindow == nullptr)
    {
        DBG("Creating new settings window");
        auto settingsContent = std::make_unique<juce::Component>();
        settingsContent->setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::white);

        int yPos = 10;

        auto addLabel = [&](const juce::String& text) {
            auto label = std::make_unique<juce::Label>();
            label->setText(text, juce::dontSendNotification);
            label->setBounds(10, yPos, 380, 20);
            settingsContent->addAndMakeVisible(label.get());
            ownedSettingsComponents.add(label.release());
            yPos += 25;
        };

        // Add labels and other components here
        addLabel("MIDI Devices:");

        // Add the scan button
        scanButton.setButtonText("Scan for MIDI Devices");
        scanButton.setBounds(10, yPos, 185, 30);
        settingsContent->addAndMakeVisible(scanButton);

        applyButton.setButtonText("Apply MIDI Selections");
        applyButton.setBounds(205, yPos, 185, 30);
        settingsContent->addAndMakeVisible(applyButton);
        yPos += 35;

        addLabel("Fade Rate:");
        fadeRateSlider.setBounds(10, yPos, 380, 30);
        settingsContent->addAndMakeVisible(fadeRateSlider);
        yPos += 35;

        disableFadeToggle.setBounds(10, yPos, 380, 30);
        settingsContent->addAndMakeVisible(disableFadeToggle);
        yPos += 35;

        addLabel("Note Color:");
        noteColorSelector.setBounds(10, yPos, 380, 300);
        settingsContent->addAndMakeVisible(noteColorSelector);
        yPos += 305;

        // Instant Update Toggle
        instantUpdateToggle.setBounds(10, yPos, 380, 30);
        settingsContent->addAndMakeVisible(instantUpdateToggle);
        yPos += 35;

        // Set the size of the content
        settingsContent->setSize(400, yPos);

        // Create and configure the settings window
        settingsWindow = std::make_unique<SettingsWindow>("Settings",
                                                          juce::Colours::white,
                                                          juce::DocumentWindow::allButtons,
                                                          this);
        settingsWindow->setContentOwned(settingsContent.release(), true);
        settingsWindow->setUsingNativeTitleBar(true);
        settingsWindow->setResizable(true, true);
        settingsWindow->centreWithSize(400, yPos);

        // Make the settings window visible
        settingsWindow->setVisible(true);

        // Scan for MIDI devices/channels when the settings window is first created
        refreshMidiInputs();
    }
    else
    {
        DBG("Bringing existing settings window to front");
        settingsWindow->toFront(true);
        settingsWindow->setVisible(true);
    }

    // Set up the closing behavior
    if (auto* settingsWin = dynamic_cast<SettingsWindow*>(settingsWindow.get()))
    {
        settingsWin->setOnCloseCallback([this]() {
            settingsWindow = nullptr;
        });
    }
}

//==============================================================================
void MainComponent::updateMidiDeviceSelections()
{
    selectedMidiDevices.clear();
    selectedChannels.clear();

    for (int i = 0; i < midiDeviceToggles.size(); ++i)
    {
        if (midiDeviceToggles[i]->getToggleState())
        {
            selectedMidiDevices.add(midiDevicesList[i]);

            auto* channelToggles = midiChannelToggles[i];
            for (int j = 0; j < channelToggles->size(); ++j)
            {
                if ((*channelToggles)[j]->getToggleState())
                {
                    selectedChannels.add(j + 1);
                }
            }
        }
    }

    DBG("Selected MIDI Devices:");
    for (const auto& device : selectedMidiDevices)
    {
        DBG(" - " + device);
    }

    DBG("Selected Channels:");
    for (const auto& channel : selectedChannels)
    {
        DBG(" - Channel " + juce::String(channel));
    }
}

//==============================================================================
void MainComponent::fadeToggleChanged()
{
    if (disableFadeToggle.getToggleState())
    {
        fadeRate = 0.0f;
    }
    else
    {
        fadeRate = static_cast<float>(fadeRateSlider.getValue());
    }

    if (instantUpdateToggle.getToggleState())
    {
        repaint(); // Apply changes immediately if instantUpdateMode is enabled
    }
}

//==============================================================================
void MainComponent::openSelectedMidiInputs()
{
    DBG("Opening selected MIDI inputs:");
    for (const auto& deviceName : selectedMidiDevices)
    {
        // Check if the device is already opened
        bool isAlreadyOpened = std::ranges::any_of(midiInputsOpened,
                                                   [&](juce::MidiInput* input) { return input->getName() == deviceName; });
        if (isAlreadyOpened)
            continue;

        auto availableDevices = juce::MidiInput::getAvailableDevices();
        auto deviceInfo = std::ranges::find_if(availableDevices, [&](const auto& d) { return d.name == deviceName; });

        if (deviceInfo != availableDevices.end())
        {
            if (auto midiInput = juce::MidiInput::openDevice(deviceInfo->identifier, this))
            {
                midiInputsOpened.add(midiInput.release());
                midiInputsOpened.getLast()->start();
                DBG("Opened and started MIDI device: " + deviceName);
            }
            else
            {
                DBG("Failed to open MIDI device: " + deviceName);
            }
        }
    }
}