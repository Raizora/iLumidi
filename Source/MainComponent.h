#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent, public juce::MidiInputCallback, public juce::Slider::Listener, public juce::ChangeListener, private juce::Timer // X
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    //==============================================================================

    // MIDI Input Callback
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    void sliderValueChanged(juce::Slider* slider) override; // X
    void changeListenerCallback(juce::ChangeBroadcaster* source) override; // X

private:
    //==============================================================================
    // Your private member variables go here...

    // Manages the state of a MIDI keyboard
    juce::MidiKeyboardState keyboardState;
    // Collects incoming MIDI messages
    juce::MidiMessageCollector midiCollector;
    // Stores MIDI messages for visualization with fade factor // X
    std::vector<std::pair<juce::MidiMessage, float>> midiMessages;

    // New member variable to hold opened MIDI inputs
    juce::OwnedArray<juce::MidiInput> midiInputsOpened;

    // New method to process MIDI messages and update visuals
    void processMidiMessage(const juce::MidiMessage& message);

    // Timer callback method
    void timerCallback() override;

    // Slider for fade rate
    juce::Slider fadeRateSlider; // X

    // Color picker for note color
    juce::ColourSelector noteColorSelector; // X

    // Method to handle color changes
    void noteColorChanged(); // X

    // Fade rate parameter
    float fadeRate; // X

    // Note color parameter
    juce::Colour noteColor; // X

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};