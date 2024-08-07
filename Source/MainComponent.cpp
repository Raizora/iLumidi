#include "MainComponent.h"
#include "CustomLookAndFeel.h"
#include <algorithm>
#include <vector>
#include <ranges>

//==============================================================================
// Definition for SettingsWindowCloseButtonHandler class
class MainComponent::SettingsWindowCloseButtonHandler : public juce::DocumentWindow
{
public:
    SettingsWindowCloseButtonHandler(const juce::String& name, juce::Colour backgroundColour, int buttons, MainComponent* owner)
        : DocumentWindow(name, backgroundColour, buttons), ownerComponent(owner)
    {
    }

    void closeButtonPressed() override
    {
        ownerComponent->settingsWindow = nullptr;
    }

private:
    MainComponent* ownerComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsWindowCloseButtonHandler)
};

//==============================================================================
// Constructor for MainComponent
MainComponent::MainComponent()
    : settingsWindow(nullptr), midiDevicesEditor(nullptr),
      keyboardState(), midiCollector(), midiMessages(), midiInputsOpened(),
      fadeRateSlider(), disableFadeToggle(), scanButton(), noteColorSelector(),
      midiDevicesList(), selectedMidiDevices(), selectedChannels(), midiDeviceToggles(), midiChannelToggles(), applyButton(),
      instantUpdateToggle(), fadeRate(5.0f), noteColor(juce::Colours::white), customLookAndFeel(), instantUpdateMode(false)
{
    setLookAndFeel(&customLookAndFeel);
    setSize(800, 600);

    instantUpdateToggle.setButtonText("Instant Color Update");
    instantUpdateToggle.setToggleState(false, juce::dontSendNotification);
    instantUpdateToggle.addListener(this);

    disableFadeToggle.setButtonText("Disable Fade");
    disableFadeToggle.addListener(this);

    enableMode1Button.setButtonText("Enable Mode 1");
    enableMode1Button.addListener(this);

    enableMode2Button.setButtonText("Enable Mode 2");
    enableMode2Button.addListener(this);
}

// New method to initialize components
void MainComponent::initialize()
{
    DBG("MainComponent initialized. Size: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));

    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio)
        && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
            [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels(2, 2);
    }

    refreshMidiInputs();

    DBG("Opening selected MIDI inputs:");
    for (const auto& deviceName : selectedMidiDevices)
    {
        auto availableDevices = juce::MidiInput::getAvailableDevices();
        auto deviceInfo = std::ranges::find_if(availableDevices, [&](const auto& d) { return d.name == deviceName; });

        if (deviceInfo != availableDevices.end())
        {
            DBG("Found device info for: " + deviceName);
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
        else
        {
            DBG("Could not find device info for: " + deviceName);
        }
    }

    fadeRateSlider.setRange(0.1, 20.0, 0.1);
    fadeRateSlider.setValue(5.0);
    fadeRateSlider.addListener(this);
    fadeRateSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::white);
    fadeRateSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
    fadeRateSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);

    disableFadeToggle.setButtonText("Disable Fade");
    disableFadeToggle.addListener(this);

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

    enableMode1Button.setButtonText("Enable Mode 1");
    enableMode1Button.addListener(this);

    enableMode2Button.setButtonText("Enable Mode 2");
    enableMode2Button.addListener(this);
}

void MainComponent::openSelectedMidiInputs()
{
    DBG("Opening selected MIDI inputs:");
    for (const auto& deviceName : selectedMidiDevices)
    {
        // Changed to iterate through available devices to find the matching one
        auto availableDevices = juce::MidiInput::getAvailableDevices();
        auto deviceInfo = std::ranges::find_if(availableDevices, [&](const auto& d) { return d.name == deviceName; });

        // Check if a valid device was found
        if (deviceInfo != availableDevices.end())
        {
            // Attempt to open the MIDI device
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
        else
        {
            DBG("Could not find device info for: " + deviceName);
        }
    }
}


//==============================================================================
// Destructor for MainComponent
MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr); // Clean up custom look and feel
    shutdownAudio();

    for (auto* device : midiInputsOpened)
    {
        if (device != nullptr)
        {
            device->stop();
        }
    }
    midiInputsOpened.clear(true);
}

//==============================================================================
// Method to prepare the audio playback
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    midiCollector.reset(sampleRate);
}

//==============================================================================
// Method to process audio blocks
void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    juce::MidiBuffer midiBuffer;
    midiCollector.removeNextBlockOfMessages(midiBuffer, bufferToFill.numSamples);

    keyboardState.processNextMidiBuffer(midiBuffer, 0, bufferToFill.numSamples, true);
}

//==============================================================================
// Method to release audio resources
void MainComponent::releaseResources()
{
}

//==============================================================================
// Method to paint the GUI
//==============================================================================
// Method to paint the GUI
void MainComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // Further reduced paint call logging
    static int paintCallCount = 0;
    static juce::uint32 lastPaintTime = 0;
    juce::uint32 currentTime = juce::Time::getMillisecondCounter();

    if (currentTime - lastPaintTime > 1000 || !midiMessages.empty())  // Log at most once per second or when there are messages
    {
        DBG("Paint called (" + juce::String(++paintCallCount) + "). Number of MIDI messages: " + juce::String(midiMessages.size()));
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

            DBG("Drawing note: " + juce::String(noteNumber) + " at x: " + juce::String(x) + " with height: " + juce::String(height));

            juce::Path triangle;
            triangle.addTriangle(x, static_cast<float>(getHeight()) - height, x + 10.0f, static_cast<float>(getHeight()), x - 10.0f, static_cast<float>(getHeight()));
            g.fillPath(triangle);

            if (!disableFadeToggle.getToggleState())
            {
                alpha *= (1.0f - fadeRate / 100.0f);
            }
        }
    }

    // Remove messages with alpha less than 0.01
    std::erase_if(midiMessages, [](const auto& pair) { return pair.second < 0.01f; });
}

//==============================================================================
// Method to handle component resizing
void MainComponent::resized()
{
}

//==============================================================================
// Method to handle note color changes
void MainComponent::noteColorChanged()
{
    noteColor = noteColorSelector.getCurrentColour();
    DBG("Note color changed to: " + noteColor.toString());

    if (instantUpdateMode) // X- Check the instant update mode
    {
        repaint(); // Repaint the component to reflect the new color immediately
    }
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    DBG("Incoming MIDI Message from: " + source->getName() + " Channel: " + juce::String(message.getChannel()));

    // Only update selections if they've changed
    static juce::String lastSelectedDevices;
    static juce::String lastSelectedChannels;

    // Replace joinIntoString with manual string construction for selectedMidiDevices
    juce::String currentSelectedDevices;
    for (const auto& device : selectedMidiDevices)
        currentSelectedDevices += device + ", ";
    // Remove trailing ", " if the string is not empty
    if (currentSelectedDevices.isNotEmpty())
        currentSelectedDevices = currentSelectedDevices.dropLastCharacters(2);

    // Replace joinIntoString with manual string construction for selectedChannels
    juce::String currentSelectedChannels;
    for (const auto& channel : selectedChannels)
        currentSelectedChannels += juce::String(channel) + ", ";
    // Remove trailing ", " if the string is not empty
    if (currentSelectedChannels.isNotEmpty())
        currentSelectedChannels = currentSelectedChannels.dropLastCharacters(2);

    // Check if the selections have changed
    if (currentSelectedDevices != lastSelectedDevices || currentSelectedChannels != lastSelectedChannels)
    {
        updateMidiDeviceSelections();
        lastSelectedDevices = currentSelectedDevices;
        lastSelectedChannels = currentSelectedChannels;

        DBG("Selected MIDI Devices: " + currentSelectedDevices);
        DBG("Selected Channels: " + currentSelectedChannels);
    }

    // Process the MIDI message if it's from a selected device and channel
    if (selectedMidiDevices.contains(source->getName()) &&
        (selectedChannels.isEmpty() || selectedChannels.contains(message.getChannel())))
    {
        DBG("Adding message to queue and processing MIDI message");
        midiCollector.addMessageToQueue(message);
        processMidiMessage(message);
    }
    else
    {
        DBG("MIDI message not from selected device/channel");
    }
}

//==============================================================================
// Method to process MIDI messages and update visuals
void MainComponent::processMidiMessage(const juce::MidiMessage& message)
{
    DBG("Processing MIDI message; Note ON: " + juce::String(message.getNoteNumber()) + ", Channel: " + juce::String(message.getChannel()));
    if (message.isNoteOn())
    {
        midiMessages.emplace_back(message, 1.0f);
        DBG("Added message to midiMessages; size: " + juce::String(midiMessages.size()));

        if (midiMessages.size() > 100)
        {
            midiMessages.erase(midiMessages.begin());
            DBG("Erased oldest message; new size: " + juce::String(midiMessages.size()));
        }
    }

    // Ensure repaint is called after processing a message
    DBG("Calling repaint after processing MIDI message");
    repaint();
}

//==============================================================================
// Method called periodically by the timer
void MainComponent::timerCallback()
{
    repaint();
}

//==============================================================================
// Method to refresh the list of available MIDI inputs
void MainComponent::refreshMidiInputs()
{
    DBG("Starting refreshMidiInputs()");
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    DBG("Refreshing MIDI Inputs:");

    // X- Update to not change the state of existing toggles
    for (const auto& input : midiInputs)
    {
        DBG("Found MIDI input: " + input.name + " (ID: " + input.identifier + ")");

        int existingIndex = midiDevicesList.indexOf(input.name);
        if (existingIndex != -1)
        {
            // Device already exists, keep its current state
            continue;
        }

        midiDevicesList.add(input.name);

        auto* deviceToggle = new juce::ToggleButton(input.name);
        deviceToggle->setColour(juce::ToggleButton::textColourId, juce::Colours::black);
        deviceToggle->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::black);
        deviceToggle->setColour(juce::ToggleButton::tickColourId, juce::Colours::black);
        deviceToggle->addListener(this);
        midiDeviceToggles.add(deviceToggle);

        auto* channelToggles = new juce::OwnedArray<juce::ToggleButton>();
        for (int i = 1; i <= 16; ++i)
        {
            auto* channelToggle = new juce::ToggleButton("Ch " + juce::String(i));
            channelToggle->setColour(juce::ToggleButton::textColourId, juce::Colours::black);
            channelToggle->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::black);
            channelToggle->setColour(juce::ToggleButton::tickColourId, juce::Colours::black);
            channelToggle->addListener(this);
            channelToggles->add(channelToggle);
        }
        midiChannelToggles.add(channelToggles);
    }

    // X- Remove any devices that are no longer present
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
    // X- Remove the call to openSelectedMidiInputs() from here
    // openSelectedMidiInputs();
}
//==============================================================================
// X- New method to apply MIDI selections
void MainComponent::applyMidiSelections()
{
    // First, close all currently open MIDI inputs
    for (auto* device : midiInputsOpened)
    {
        device->stop();
    }
    midiInputsOpened.clear();

    // Now open the newly selected MIDI inputs
    for (const auto& deviceName : selectedMidiDevices)
    {
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

//==============================================================================
// Method to handle slider value changes
void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &fadeRateSlider)
    {
        // X- Explicitly cast the double value to float to avoid narrowing conversion warning.
        fadeRate = static_cast<float>(fadeRateSlider.getValue());
    }
}

//==============================================================================
// Method to handle color picker changes
void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &noteColorSelector)
    {
        noteColorChanged(); // Call noteColorChanged when the color changes
    }
}

//==============================================================================
// Override the Button::Listener method to handle toggle button changes
//==============================================================================
// Override the Button::Listener method to handle toggle button changes
void MainComponent::buttonClicked(juce::Button* button)
{
    DBG("Button clicked: " + button->getName());

    // Check if the clicked button is one of our MIDI device or channel toggles
    bool isMidiToggle = false;
    for (auto* deviceToggle : midiDeviceToggles)
    {
        if (button == deviceToggle)
        {
            DBG("MIDI device toggle clicked: " + deviceToggle->getName());
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
                    DBG("MIDI channel toggle clicked: " + channelToggle->getName());
                    isMidiToggle = true;
                    break;
                }
            }
            if (isMidiToggle) break;
        }
    }

    if (isMidiToggle)
    {
        DBG("Updating MIDI device selections");
        updateMidiDeviceSelections();
    }
    else if (button == &disableFadeToggle)
    {
        DBG("Fade toggle button clicked");
        fadeToggleChanged();
    }
    else if (button == &scanButton)
    {
        DBG("Scan button clicked");
        refreshMidiInputs();
    }
    else if (button == &applyButton)
    {
        DBG("Apply button clicked");
        updateMidiDeviceSelections();
        applyMidiSelections();
    }
    else if (button == &instantUpdateToggle)
    {
        DBG("Instant update toggle button clicked");
        instantUpdateMode = instantUpdateToggle.getToggleState();
        if (instantUpdateMode)
        {
            repaint();
        }
    }
    else if (button == &enableMode1Button)
    {
        DBG("Enable Mode 1 button clicked");
        // Add logic to handle enabling Mode 1
    }
    else if (button == &enableMode2Button)
    {
        DBG("Enable Mode 2 button clicked");
        // Add logic to handle enabling Mode 2
    }

    DBG("Button click handling completed");
}

//==============================================================================
void MainComponent::showSettingsWindow()
{
    DBG("Showing settings window");
    if (settingsWindow == nullptr)
    {
        DBG("Creating new settings window");
        auto settingsContent = std::make_unique<juce::Component>();
        settingsContent->setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::white);

        // Add components to settingsContent
        int yPos = 10;

        auto addLabel = [&](const juce::String& text) {
            auto label = std::make_unique<juce::Label>();
            label->setText(text, juce::dontSendNotification);
            label->setBounds(10, yPos, 380, 20);
            settingsContent->addAndMakeVisible(*label);
            yPos += 25;
            return label.release();
        };

        addLabel("MIDI Devices:");

        DBG("Adding MIDI device and channel toggles");
        for (int i = 0; i < midiDeviceToggles.size(); ++i)
        {
            auto* deviceToggle = midiDeviceToggles[i];
            deviceToggle->setBounds(10, yPos, 380, 20);
            settingsContent->addAndMakeVisible(deviceToggle);
            yPos += 25;

            auto* channelToggles = midiChannelToggles[i];
            for (int j = 0; j < channelToggles->size(); ++j)
            {
                auto* channelToggle = (*channelToggles)[j];
                channelToggle->setBounds(30 + (j % 8) * 45, yPos, 40, 20);
                settingsContent->addAndMakeVisible(channelToggle);

                if (j % 8 == 7)
                {
                    yPos += 25;
                }
            }
            yPos += 30;
        }

        DBG("Adding other controls");
        scanButton.setBounds(10, yPos, 185, 30);
        settingsContent->addAndMakeVisible(scanButton);
        // X- Add APPLY button
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

        DBG("Setting content size");
        settingsContent->setSize(400, yPos);

        // Changed to use a custom DocumentWindow subclass
        class SettingsWindow : public juce::DocumentWindow
        {
        public:
            SettingsWindow(const juce::String& name, juce::Colour backgroundColour, int buttonsNeeded, MainComponent* owner)
                : DocumentWindow(name, backgroundColour, buttonsNeeded), owner(owner)
            {
            }

            void closeButtonPressed() override
            {
                DBG("Settings window close button pressed");
                owner->settingsWindow = nullptr;
            }

        private:
            MainComponent* owner;
        };

        DBG("Creating SettingsWindow");
        settingsWindow = std::make_unique<SettingsWindow>("Settings",
                                                          juce::Colours::white,
                                                          juce::DocumentWindow::allButtons,
                                                          this);
        settingsWindow->setContentOwned(settingsContent.release(), true);
        settingsWindow->setUsingNativeTitleBar(true);
        settingsWindow->setResizable(true, true);
        settingsWindow->centreWithSize(400, yPos);

        DBG("Making settings window visible");
        settingsWindow->setVisible(true);
    }
    else
    {
        DBG("Bringing existing settings window to front");
        settingsWindow->toFront(true);
    }
}

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

        int yPos = 10;

        auto addLabel = [&](const juce::String& text) {
            auto label = std::make_unique<juce::Label>();
            label->setText(text, juce::dontSendNotification);
            label->setBounds(10, yPos, 380, 20);
            content->addAndMakeVisible(*label);
            yPos += 25;
            return label.release();
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

        // Mode Buttons
        DBG("Adding Mode buttons");
        enableMode1Button.setBounds(10, yPos, 185, 30);
        content->addAndMakeVisible(enableMode1Button);
        enableMode2Button.setBounds(205, yPos, 185, 30);
        content->addAndMakeVisible(enableMode2Button);
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
// Method to update the MIDI device selections
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
// Method to handle fade toggle changes
void MainComponent::fadeToggleChanged()
{
    if (disableFadeToggle.getToggleState())
    {
        fadeRate = 0.0f; // Set fade rate to 0 if toggled on
    }

}