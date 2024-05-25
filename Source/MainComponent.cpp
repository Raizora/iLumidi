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

    // Log available MIDI input devices
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    DBG("Available MIDI Inputs:");
    for (const auto& input : midiInputs)
    {
        DBG(input.identifier + ": " + input.name);
    }

    // Initialize MIDI input
    for (const auto& input : midiInputs)
    {
        std::unique_ptr<juce::MidiInput> device = juce::MidiInput::openDevice(input.identifier, this);
        if (device != nullptr)
        {
            device->start();
            midiInputsOpened.add(device.release());
        }
    }

    // Set up the fade rate slider
    fadeRateSlider.setRange(1.0, 10.0, 0.1);
    fadeRateSlider.setValue(5.0);
    fadeRateSlider.addListener(this);
    addAndMakeVisible(fadeRateSlider);

    fadeRate = fadeRateSlider.getValue();

    // Set up the color picker
    noteColorSelector.setCurrentColour(juce::Colours::white); // X
    noteColorSelector.addChangeListener(this); // X
    addAndMakeVisible(noteColorSelector); // X

    noteColor = noteColorSelector.getCurrentColour(); // X

    startTimerHz(30); // Start a timer to repaint the component regularly
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

            float x = (float)(getWidth() * noteNumber / 127.0); // Position based on note number
            float height = getHeight() * velocity / 127.0; // Height based on velocity

            // Calculate color based on user selection and note number
            juce::Colour noteColour = noteColor.withAlpha(alpha); // X

            g.setColour(noteColour);

            // Draw a triangle
            juce::Path triangle;
            triangle.addTriangle(x, static_cast<float>(getHeight()) - height, x + 10.0f, static_cast<float>(getHeight()), x - 10.0f, static_cast<float>(getHeight()));
            g.fillPath(triangle);

            // Reduce alpha for fading effect
            alpha *= (1.0f - fadeRate / 100.0f);
        }
    }

    // Remove faded out messages
    std::erase_if(midiMessages, [](const std::pair<juce::MidiMessage, float>& pair) { return pair.second < 0.01f; });
}

//==============================================================================
// Method to handle component resizing
void MainComponent::resized()
{
    fadeRateSlider.setBounds(10, 10, getWidth() - 20, 30);
    // Adjust position and size as needed
    noteColorSelector.setBounds(10, 50, getWidth() - 20, 300); // X
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
    repaint(); // Repaint the component to refresh the display
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