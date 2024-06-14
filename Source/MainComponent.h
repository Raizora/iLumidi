#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent, public juce::MidiInputCallback, public juce::Slider::Listener, public juce::ChangeListener, public juce::Button::Listener, private juce::Timer
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
    void sliderValueChanged(juce::Slider* slider) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void buttonClicked(juce::Button* button) override;
    void fadeToggleChanged();

    //==============================================================================

    // Method to show the settings window
    void showSettingsWindow();

private:
    //==============================================================================
    // Your private member variables go here...

    // Settings window
    std::unique_ptr<juce::DocumentWindow> settingsWindow;
    std::unique_ptr<juce::TextEditor> midiDevicesEditor; // Add this member variable

    // Method to refresh the list of available MIDI inputs
    void refreshMidiInputs();

    // Manages the state of a MIDI keyboard
    juce::MidiKeyboardState keyboardState;
    // Collects incoming MIDI messages
    juce::MidiMessageCollector midiCollector;
    // Stores MIDI messages for visualization with fade factor
    std::vector<std::pair<juce::MidiMessage, float>> midiMessages;

    // New member variable to hold opened MIDI inputs
    juce::OwnedArray<juce::MidiInput> midiInputsOpened;

    // New method to process MIDI messages and update visuals
    void processMidiMessage(const juce::MidiMessage& message);

    // Timer callback method
    void timerCallback() override;

    // Slider for fade rate
    juce::Slider fadeRateSlider;

    // Toggle button to disable fading
    juce::ToggleButton disableFadeToggle;

    // Color picker for note color
    juce::ColourSelector noteColorSelector;

    // List of MIDI devices
    juce::StringArray midiDevicesList;

    // Method to handle color changes
    void noteColorChanged();

    // Fade rate parameter
    float fadeRate;

    // Note color parameter
    juce::Colour noteColor;

    // Add this class declaration after the MainComponent class
    class SettingsWindowCloseButtonHandler : public juce::DocumentWindow
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};