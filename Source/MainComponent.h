#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h" // X- Include custom look and feel

//==============================================================================
class MainComponent : public juce::AudioAppComponent, public juce::MidiInputCallback, public juce::Slider::Listener, public juce::ChangeListener, public juce::Button::Listener, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    // Audio Block Methods
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    // Painting Methods
    void paint(juce::Graphics& g) override;
    void resized() override;

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void buttonClicked(juce::Button* button) override;
    void fadeToggleChanged();

    void showSettingsWindow();

private:

    class SettingsWindowCloseButtonHandler;

    std::unique_ptr<juce::DocumentWindow> settingsWindow;
    std::unique_ptr<juce::TextEditor> midiDevicesEditor;

    // Midi device selection
    void refreshMidiInputs();
    void refreshSettingsWindow();
    void openSelectedMidiInputs();
    void applyMidiSelections();

    // Midi input handling
    juce::MidiKeyboardState keyboardState;
    juce::MidiMessageCollector midiCollector;
    std::vector<std::pair<juce::MidiMessage, float>> midiMessages;
    juce::OwnedArray<juce::MidiInput> midiInputsOpened;

    void processMidiMessage(const juce::MidiMessage& message);
    void timerCallback() override;

    // UI
    juce::Slider fadeRateSlider;
    juce::ToggleButton disableFadeToggle;
    juce::TextButton scanButton;
    juce::ColourSelector noteColorSelector;
    juce::StringArray midiDevicesList;
    juce::Array<juce::String> selectedMidiDevices;
    juce::Array<int> selectedChannels;
    juce::OwnedArray<juce::ToggleButton> midiDeviceToggles;
    juce::OwnedArray<juce::OwnedArray<juce::ToggleButton>> midiChannelToggles;
    juce::TextButton applyButton;


    void updateMidiDeviceSelections();

    void noteColorChanged();

    float fadeRate;
    juce::Colour noteColor;

    CustomLookAndFeel customLookAndFeel; // Ensure this is a member variable

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};