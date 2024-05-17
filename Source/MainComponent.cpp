#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
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
    for (const auto& input : midiInputs) // Updated loop variable to be a const reference
    {
        std::unique_ptr<juce::MidiInput> device = juce::MidiInput::openDevice(input.identifier, this);
        if (device != nullptr)
        {
            device->start();
            midiInputsOpened.add(device.release()); // Use release() to avoide ownership issues
        }
    }

    // Start a timer to repaint the component regularly
    //startTimerHz(30); // Repaint at 30 Hz
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();

    // Close MIDI inputs
    for (auto* device : midiInputsOpened)
        delete device;
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()

    midiCollector.reset(sampleRate); // Initialize the MIDI collector with the sample rate

}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();

    // Create a MidiBuffer and pass it to removeNextBlockOfMessages
    juce::MidiBuffer midiBuffer; // Highlighted change
    midiCollector.removeNextBlockOfMessages(midiBuffer, bufferToFill.numSamples); // Highlighted change

    // Add the collected MIDI messages to the keyboard state
    keyboardState.processNextMidiBuffer(midiBuffer, 0, bufferToFill.numSamples, true); // Highlighted change
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

//==============================================================================
// MIDI Input Callback implementation
void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    midiCollector.addMessageToQueue(message); // Add incoming MIDI messages to the collector
}