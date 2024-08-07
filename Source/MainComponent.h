#pragma once

#include <JuceHeader.h> // X- Include the JUCE framework header
#include "CustomLookAndFeel.h" // X- Include custom look and feel

//==============================================================================
// X- Defines the MainComponent class, which inherits from several JUCE classes for handling audio, MIDI, GUI components, and timers
class MainComponent : public juce::AudioAppComponent, public juce::MidiInputCallback, public juce::Slider::Listener, public juce::ChangeListener, public juce::Button::Listener, private juce::Timer
{
public:
    MainComponent(); // X- Constructor
    ~MainComponent() override; // X- Destructor

    void initialize(); // New method to initialize components

    // X- Audio Block Methods
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override; // X- Prepares the audio playback
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override; // X- Processes the next audio block
    void releaseResources() override; // X- Releases audio resources

    // X- Painting Methods
    void paint(juce::Graphics& g) override; // X- Paints the GUI
    void resized() override; // X- Handles component resizing

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override; // X- Handles incoming MIDI messages
    void sliderValueChanged(juce::Slider* slider) override; // X- Handles slider value changes
    void changeListenerCallback(juce::ChangeBroadcaster* source) override; // X- Handles changes in the color selector
    void buttonClicked(juce::Button* button) override; // X- Handles button clicks
    void fadeToggleChanged(); // X- Handles fade toggle changes
    void showSettingsWindow(); // X- Shows the settings window
    void noteColorChanged();

private:
    // Inner class to handle the close button of the settings window
    class SettingsWindowCloseButtonHandler;

    std::unique_ptr<juce::DocumentWindow> settingsWindow; // Pointer to the settings window
    std::unique_ptr<juce::TextEditor> midiDevicesEditor; // Pointer to the text editor for MIDI devices

    // Methods for MIDI device selection
    void refreshMidiInputs(); // Refreshes the list of available MIDI inputs
    void refreshSettingsWindow(); // Refreshes the settings window
    void openSelectedMidiInputs(); // Opens the selected MIDI inputs
    void applyMidiSelections(); // Applies the selected MIDI inputs and channels

    // Members for MIDI input handling
    juce::MidiKeyboardState keyboardState; // State of the MIDI keyboard
    juce::MidiMessageCollector midiCollector; // Collects MIDI messages
    std::vector<std::pair<juce::MidiMessage, float>> midiMessages; // Stores MIDI messages and their alpha values
    juce::OwnedArray<juce::MidiInput> midiInputsOpened; // Stores the opened MIDI inputs

    void processMidiMessage(const juce::MidiMessage& message); // Processes a MIDI message
    void timerCallback() override; // Callback method for the timer

    // UI components
    juce::Slider fadeRateSlider; // Slider for fade rate
    juce::ToggleButton disableFadeToggle; // Toggle button to disable fade
    juce::TextButton scanButton; // Button to scan for MIDI devices
    juce::ColourSelector noteColorSelector; // Selector for note color
    juce::StringArray midiDevicesList; // List of MIDI devices
    juce::Array<juce::String> selectedMidiDevices; // X- Selected MIDI devices
    juce::Array<int> selectedChannels; // Selected MIDI channels
    juce::OwnedArray<juce::ToggleButton> midiDeviceToggles; // Toggle buttons for MIDI devices
    juce::OwnedArray<juce::OwnedArray<juce::ToggleButton>> midiChannelToggles; // Toggle buttons for MIDI channels
    juce::TextButton applyButton; // Button to apply MIDI selections

    juce::ToggleButton instantUpdateToggle; // X- New toggle button for instant color update mode

    // New buttons for mode switching
    juce::TextButton enableMode1Button; // Button to enable Mode 1
    juce::TextButton enableMode2Button; // Button to enable Mode 2

    void updateMidiDeviceSelections(); // Updates the selected MIDI devices and channels

    float fadeRate; // Fade rate value
    juce::Colour noteColor; // Note color value

    CustomLookAndFeel customLookAndFeel; // Custom look and feel
    bool instantUpdateMode; // X- New flag for tracking the update mode

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent) // Prevents copying of the MainComponent class
};