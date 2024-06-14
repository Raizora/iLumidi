#include "MainComponent.h"

//==============================================================================
// Constructor for MainComponent
MainComponent::MainComponent()
{
    setSize(800, 600);

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

    startTimerHz(30); // Start a timer to repaint the component regularly
    startTimer(5000); // Start a timer to refresh MIDI inputs every 5 seconds
}

//==============================================================================
// Destructor for MainComponent
MainComponent::~MainComponent()
{
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

            float x = static_cast<float>(getWidth()) * noteNumber / 127.0f; // Position based on note number
            float height = static_cast<float>(getHeight()) * velocity / 127.0f; // Height based on velocity

            // Calculate color based on user selection and note number
            juce::Colour noteColour = noteColor.withAlpha(alpha);

            g.setColour(noteColour);

            // Draw a triangle
            juce::Path triangle;
            triangle.addTriangle(x, static_cast<float>(getHeight()) - height, x + 10.0f, static_cast<float>(getHeight()), x - 10.0f, static_cast<float>(getHeight()));
            g.fillPath(triangle);

            // Reduce alpha for fading effect if fade is enabled
            if (!disableFadeToggle.getToggleState())
            {
                alpha *= (1.0f - fadeRate / 100.0f);
            }
        }
    }

    // Remove faded out messages
    std::erase_if(midiMessages, [](const std::pair<juce::MidiMessage, float>& pair) { return pair.second < 0.01f; });
}

//==============================================================================
// Method to handle component resizing
void MainComponent::resized()
{
    // No need to add MIDI devices label and editor here
}

//==============================================================================
// Method to handle incoming MIDI messages
void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    midiCollector.addMessageToQueue(message);
    processMidiMessage(message);
}

//==============================================================================
// Method to process MIDI messages and update visuals
void MainComponent::processMidiMessage(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        midiMessages.emplace_back(message, 1.0f); // Store message with initial alpha value of 1.0f

        // Limit the size of the stored messages to avoid memory issues
        if (midiMessages.size() > 100)
            midiMessages.erase(midiMessages.begin());
    }
    // Ensure repaint is called on the message thread
    juce::MessageManager::callAsync([this] { repaint(); });
}

//==============================================================================
// Method called periodically by the timer
void MainComponent::timerCallback()
{
    static int counter = 0;
    counter++;

    if (counter % 6 == 0) // Refresh MIDI inputs every 5 seconds
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
    for (const auto& input : midiInputs)
    {
        DBG(input.identifier + ": " + input.name);
        midiDevicesList.add(input.name);
    }

    for (auto* device : midiInputsOpened)
    {
        if (device != nullptr)
        {
            device->stop();
        }
    }
    midiInputsOpened.clear(true);

    for (const auto& input : midiInputs)
    {
        std::unique_ptr<juce::MidiInput> device = juce::MidiInput::openDevice(input.identifier, this);
        if (device != nullptr)
        {
            device->start();
            midiInputsOpened.add(device.release());
        }
    }
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

        // Set the background color for the settings content
        settingsContent->setColour(juce::ResizableWindow::backgroundColourId, getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

        // Add MIDI devices list
        auto midiDevicesLabel = std::make_unique<juce::Label>("MIDI Devices Label", "MIDI Devices:");
        midiDevicesLabel->setBounds(10, 10, 380, 30);
        settingsContent->addAndMakeVisible(*midiDevicesLabel);

        midiDevicesEditor = std::make_unique<juce::TextEditor>();
        midiDevicesEditor->setMultiLine(true);
        midiDevicesEditor->setReadOnly(true);
        midiDevicesEditor->setBounds(10, 50, 380, 100);
        midiDevicesEditor->setColour(juce::TextEditor::backgroundColourId, getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        midiDevicesEditor->setColour(juce::TextEditor::textColourId, juce::Colours::white);
        for (const auto& device : midiDevicesList)
        {
            midiDevicesEditor->insertTextAtCaret(device + "\n");
        }
        settingsContent->addAndMakeVisible(*midiDevicesEditor);

        int yPos = 160;

        // Move existing settings controls to the settings window
        fadeRateSlider.setBounds(10, yPos, 380, 30);
        settingsContent->addAndMakeVisible(fadeRateSlider);
        yPos += 40;

        disableFadeToggle.setBounds(10, yPos, 380, 30);
        settingsContent->addAndMakeVisible(disableFadeToggle);
        yPos += 40;

        noteColorSelector.setBounds(10, yPos, 380, 300);
        settingsContent->addAndMakeVisible(noteColorSelector);
        yPos += 310; // Adjusting for the height of noteColorSelector

        // Set the size of settingsContent based on its children components
        int contentHeight = yPos;
        settingsContent->setSize(400, contentHeight);  // Set size based on the content height

        settingsWindow = std::make_unique<SettingsWindowCloseButtonHandler>("Settings", juce::Colours::lightgrey, juce::DocumentWindow::allButtons, this);
        settingsWindow->setContentOwned(settingsContent.release(), true);
        settingsWindow->centreWithSize(400, contentHeight);   // Centre the window with appropriate width and height
        settingsWindow->setVisible(true);
    }
    else
    {
        settingsWindow->toFront(true);
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