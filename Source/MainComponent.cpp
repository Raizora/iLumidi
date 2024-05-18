#include "MainComponent.h"

//==============================================================================
// Constructor for MainComponent
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after you add any child components.
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    // Initialize MIDI input
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    for (const auto& input : midiInputs) // X
    {
        std::unique_ptr<juce::MidiInput> device = juce::MidiInput::openDevice(input.identifier, this);
        if (device != nullptr)
        {
            device->start();
            midiInputsOpened.add(device.release()); // Use release() to avoid ownership issues X
        }
    }

    // Start a timer to repaint the component regularly
    //startTimerHz(30); // Repaint at 30 Hz
}

//==============================================================================
// Destructor for MainComponent
MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();

    // Properly stop and delete MIDI inputs
    for (auto* device : midiInputsOpened)
    {
        if (device != nullptr) // Ensure the device is not null X
        {
            device->stop();  // Stop the MIDI input device X
        }
    }
    midiInputsOpened.clear(true);  // Clear the OwnedArray and delete the devices X
}

//==============================================================================
// Method to prepare the audio playback
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    midiCollector.reset(sampleRate); // Initialize the MIDI collector with the sample rate
}

//==============================================================================
// Method to process audio blocks
void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    juce::MidiBuffer midiBuffer; // X
    midiCollector.removeNextBlockOfMessages(midiBuffer, bufferToFill.numSamples); // X

    keyboardState.processNextMidiBuffer(midiBuffer, 0, bufferToFill.numSamples, true); // X
}

//==============================================================================
// Method to release audio resources
void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being restarted due to a setting change.
}

//==============================================================================
// Method to paint the GUI
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    // You can add your drawing code here!
}

//==============================================================================
// Method to handle component resizing
void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should update their positions.
}

//==============================================================================
// Method to handle incoming MIDI messages
void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    midiCollector.addMessageToQueue(message); // Add incoming MIDI messages to the collector
}