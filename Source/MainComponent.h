#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"

//==============================================================================
class MainComponent : public juce::Component,
                      public juce::MidiInputCallback,
                      public juce::Slider::Listener,
                      public juce::ChangeListener,
                      public juce::Button::Listener,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void initialize();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void buttonClicked(juce::Button* button) override;

    void showSettingsWindow();

private:
    // Inner class to handle the close button of the settings window
    class SettingsWindowCloseButtonHandler;

    std::unique_ptr<juce::DocumentWindow> settingsWindow;

    // Methods for MIDI device selection
    void refreshMidiInputs();
    void refreshSettingsWindow();
    void openSelectedMidiInputs();
    void applyMidiSelections();
    void updateMidiDeviceSelections();
    void processMidiMessage(const juce::MidiMessage& message);
    void timerCallback() override;

    // **Added missing method declarations**
    void noteColorChanged();
    void fadeToggleChanged();

    // UI components
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

    juce::ToggleButton instantUpdateToggle;

    // New buttons for mode switching
    juce::TextButton enableMode1Button;
    juce::TextButton enableMode2Button;

    float fadeRate;
    juce::Colour noteColor;

    CustomLookAndFeel customLookAndFeel;
    bool instantUpdateMode;

    // MIDI data storage
    std::vector<std::pair<juce::MidiMessage, float>> midiMessages;

    // **Added missing variable declaration**
    juce::OwnedArray<juce::MidiInput> midiInputsOpened;

    // **Added OwnedArray to manage dynamically created components**
    juce::OwnedArray<juce::Component> ownedSettingsComponents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};