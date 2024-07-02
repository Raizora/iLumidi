#include "MainComponent.h"
#include "CustomLookAndFeel.h"

CustomLookAndFeel customLookAndFeel; // X- Instantiate custom look and feel

//==============================================================================
// Constructor for MainComponent
MainComponent::MainComponent()
{
    setLookAndFeel(&customLookAndFeel); // X- Set custom look and feel
    setSize(800, 600);

    // Check for audio recording permissions and initialize audio channels
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

    // Initialize MIDI input
    refreshMidiInputs(); // Initial setup of MIDI inputs

    // Set up the fade rate slider
    fadeRateSlider.setRange(1.0, 20.0, 0.1); // Updated range
    fadeRateSlider.setValue(5.0);
    fadeRateSlider.addListener(this);

    // Set up the fade toggle button
    disableFadeToggle.setButtonText("Disable Fade");
    disableFadeToggle.addListener(this);

    fadeRate = fadeRateSlider.getValue();

    // Set up the color picker
    noteColorSelector.setCurrentColour(juce::Colours::white);
    noteColorSelector.addChangeListener(this);

    noteColor = noteColorSelector.getCurrentColour();

    // Start timers
    startTimerHz(30); // Start a timer to repaint the component regularly
    startTimer(5000); // Start a timer to refresh MIDI inputs every 5 seconds
}

//==============================================================================
// Destructor for MainComponent
MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr); // X- Clean up custom look and feel
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
void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    for (auto& pair : midiMessages)
    {
        auto& message = pair.first;
        float& alpha = pair.second;
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            float velocity = message.getVelocity();

            float x = static_cast<float>(getWidth()) * noteNumber / 127.0f;
            float height = static_cast<float>(getHeight()) * velocity / 127.0f;

            juce::Colour noteColour = noteColor.withAlpha(alpha);

            g.setColour(noteColour);

            juce::Path triangle;
            triangle.addTriangle(x, static_cast<float>(getHeight()) - height, x + 10.0f, static_cast<float>(getHeight()), x - 10.0f, static_cast<float>(getHeight()));
            g.fillPath(triangle);

            if (!disableFadeToggle.getToggleState())
            {
                alpha *= (1.0f - fadeRate / 100.0f);
            }
        }
    }

    std::erase_if(midiMessages, [](const std::pair<juce::MidiMessage, float>& pair) { return pair.second < 0.01f; });
}

//==============================================================================
// Method to handle component resizing
void MainComponent::resized()
{
}

//==============================================================================
// Method to handle incoming MIDI messages
void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    updateMidiDeviceSelections();  // Ensure selections are updated

    if (selectedMidiDevices.contains(source->getName()) &&
        (selectedChannels.isEmpty() || selectedChannels.contains(message.getChannel())))
    {
        midiCollector.addMessageToQueue(message);
        processMidiMessage(message);
    }
}

//==============================================================================
// Method to process MIDI messages and update visuals
void MainComponent::processMidiMessage(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        midiMessages.emplace_back(message, 1.0f);

        if (midiMessages.size() > 100)
            midiMessages.erase(midiMessages.begin());
    }

    juce::MessageManager::callAsync([this] { repaint(); });
}

//==============================================================================
// Method called periodically by the timer
void MainComponent::timerCallback()
{
    static int counter = 0;
    counter++;

    if (counter % 6 == 0)
    {
        refreshMidiInputs();
    }

    repaint();
}

//==============================================================================
// Method to refresh the list of available MIDI inputs
void MainComponent::refreshMidiInputs()
{
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    DBG("Refreshing MIDI Inputs:");
    midiDevicesList.clear();
    midiDeviceToggles.clear();
    midiChannelToggles.clear();

    for (const auto& input : midiInputs)
    {
        DBG(input.identifier + ": " + input.name);
        midiDevicesList.add(input.name);

        // Create a toggle button for the MIDI device
        auto* deviceToggle = new juce::ToggleButton(input.name);
        deviceToggle->setColour(juce::ToggleButton::textColourId, juce::Colours::black); // Set text color to black
        deviceToggle->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::black);
        deviceToggle->setColour(juce::ToggleButton::tickColourId, juce::Colours::black); // X- Ensure the tick mark is visible
        midiDeviceToggles.add(deviceToggle);

        // Create an array of toggle buttons for the MIDI channels
        auto* channelToggles = new juce::OwnedArray<juce::ToggleButton>();
        for (int i = 1; i <= 16; ++i)
        {
            auto* channelToggle = new juce::ToggleButton("Ch " + juce::String(i));
            channelToggle->setColour(juce::ToggleButton::textColourId, juce::Colours::black); // Set text color to black
            channelToggle->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::black);
            channelToggle->setColour(juce::ToggleButton::tickColourId, juce::Colours::black); // X- Ensure the tick mark is visible
            channelToggles->add(channelToggle);
        }
        midiChannelToggles.add(channelToggles);
    }
    juce::MessageManager::callAsync([this] { refreshSettingsWindow(); }); // X- Use callAsync to ensure safe execution
}

//==============================================================================
// Method to handle slider value changes
void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &fadeRateSlider)
    {
        fadeRate = fadeRateSlider.getValue();
    }
}

//==============================================================================
// Method to handle color picker changes
void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &noteColorSelector)
    {
        noteColor = noteColorSelector.getCurrentColour();
    }
}

//==============================================================================
// Method to handle button click events
void MainComponent::buttonClicked(juce::Button* button)
{
    if (button == &disableFadeToggle)
    {
        fadeToggleChanged();
    }
}

//==============================================================================
// Method to show the settings window
void MainComponent::showSettingsWindow()
{
    if (settingsWindow == nullptr)
    {
        auto settingsContent = std::make_unique<juce::Component>();
        settingsContent->setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::white);

        // Add MIDI devices list
        auto midiDevicesLabel = std::make_unique<juce::Label>("MIDI Devices Label", "MIDI Devices:");
        midiDevicesLabel->setBounds(10, 10, 380, 30);
        settingsContent->addAndMakeVisible(*midiDevicesLabel);

        int yPos = 50;

        // Add checkboxes for each MIDI device and its channels
        for (int i = 0; i < midiDevicesList.size(); ++i)
        {
            auto* deviceToggle = midiDeviceToggles[i];
            deviceToggle->setBounds(10, yPos, 380, 30);
            settingsContent->addAndMakeVisible(deviceToggle);
            yPos += 40;

            auto* channelToggles = midiChannelToggles[i];
            for (int j = 0; j < channelToggles->size(); ++j)
            {
                auto* channelToggle = (*channelToggles)[j];
                channelToggle->setBounds(30 + (j % 8) * 45, yPos, 40, 30);
                settingsContent->addAndMakeVisible(channelToggle);

                if (j % 8 == 7)
                {
                    yPos += 40;
                }
            }
            yPos += 40;
        }

        // Move existing settings controls to the settings window
        fadeRateSlider.setBounds(10, yPos, 380, 30);
        settingsContent->addAndMakeVisible(fadeRateSlider);
        yPos += 40;

        disableFadeToggle.setBounds(10, yPos, 380, 30);
        settingsContent->addAndMakeVisible(disableFadeToggle);
        yPos += 40;

        noteColorSelector.setBounds(10, yPos, 380, 300);
        settingsContent->addAndMakeVisible(noteColorSelector);
        yPos += 310;

        // Set the size of settingsContent based on its children components
        int contentHeight = yPos;
        settingsContent->setSize(400, contentHeight);

        auto viewport = std::make_unique<juce::Viewport>(); // X- Create a viewport for the settings content
        viewport->setViewedComponent(settingsContent.release(), true); // X- Set the content of the viewport
        settingsWindow = std::make_unique<SettingsWindowCloseButtonHandler>("Settings", juce::Colours::white, juce::DocumentWindow::allButtons, this);
        settingsWindow->setContentOwned(viewport.release(), true); // X- Set the viewport as the content of the window

        settingsWindow->centreWithSize(400, 400); // Adjusted the size
        settingsWindow->setResizable(true, true); // Ensure both directions are resizable

        settingsWindow->setLookAndFeel(&customLookAndFeel); // Set custom look and feel for the settings window
        settingsWindow->setVisible(true);
    }
    else
    {
        settingsWindow->toFront(true);
    }
}

//==============================================================================
// Method to refresh the settings window
void MainComponent::refreshSettingsWindow()
{
    if (settingsWindow != nullptr)
    {
        auto* content = dynamic_cast<juce::Viewport*>(settingsWindow->getContentComponent())->getViewedComponent();
        delete content;  // Delete the old content
        auto settingsContent = std::make_unique<juce::Component>();
        settingsContent->setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::white);

        // Add MIDI devices list
        auto midiDevicesLabel = std::make_unique<juce::Label>("MIDI Devices Label", "MIDI Devices:");
        midiDevicesLabel->setBounds(10, 10, 380, 30);
        settingsContent->addAndMakeVisible(*midiDevicesLabel);

        int yPos = 50;

        // Add checkboxes for each MIDI device and its channels
        for (int i = 0; i < midiDevicesList.size(); ++i)
        {
            auto* deviceToggle = midiDeviceToggles[i];
            deviceToggle->setBounds(10, yPos, 380, 30);
            settingsContent->addAndMakeVisible(deviceToggle);
            yPos += 40;

            auto* channelToggles = midiChannelToggles[i];
            for (int j = 0; j < channelToggles->size(); ++j)
            {
                auto* channelToggle = (*channelToggles)[j];
                channelToggle->setBounds(30 + (j % 8) * 45, yPos, 40, 30);
                settingsContent->addAndMakeVisible(channelToggle);

                if (j % 8 == 7)
                {
                    yPos += 40;
                }
            }
            yPos += 40;
        }

        // Move existing settings controls to the settings window
        fadeRateSlider.setBounds(10, yPos, 380, 30);
        settingsContent->addAndMakeVisible(fadeRateSlider);
        yPos += 40;

        disableFadeToggle.setBounds(10, yPos, 380, 30);
        settingsContent->addAndMakeVisible(disableFadeToggle);
        yPos += 40;

        noteColorSelector.setBounds(10, yPos, 380, 300);
        settingsContent->addAndMakeVisible(noteColorSelector);
        yPos += 310;

        settingsContent->setSize(400, yPos);

        auto* viewport = dynamic_cast<juce::Viewport*>(settingsWindow->getContentComponent());
        viewport->setViewedComponent(settingsContent.release(), true); // Update the content of the viewport
    }
}

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
        DBG(device);
    }

    DBG("Selected Channels:");
    for (const auto& channel : selectedChannels)
    {
        DBG(channel);
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
    else
    {
        fadeRate = fadeRateSlider.getValue(); // Restore fade rate from slider
    }
}