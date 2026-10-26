#pragma once

#include <JuceHeader.h>
#include <functional>
#include <memory>
#include <vector>
#include "PluginProcessor.h"
#include "BSIIPresetSync.h"
#include "BSIIMicroTunePage.h"
#include "BSIIBankPage.h"
#include "BSIIAfxPage.h"

class ParameterControl : public juce::Component
{
public:
    ParameterControl (juce::AudioProcessorValueTreeState& state,
                      const bsii::ParameterSpec& parameterSpec,
                      int index);
    void resized() override;
    void setFromHardware (int value);
    int specIndex() const { return index; }
private:
    void updateDiscreteSwitch();
    int index;
    const bsii::ParameterSpec& spec;
    juce::Label label;
    juce::Slider slider;
    juce::TextButton firstChoice, secondChoice;
    bool isOctaveSwitch = false;
    bool isSyncSwitch = false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class ChoiceControl : public juce::Component
{
public:
    ChoiceControl (juce::AudioProcessorValueTreeState& state,
                   const bsii::ChoiceSpec& choiceSpec,
                   int index);
    void resized() override;
    bool setFromWireValue (int wireValue);
    int choiceIndex() const { return index; }
    void setInteractionEnabled (bool enabled);
private:
    int index;
    const bsii::ChoiceSpec& spec;
    juce::Label label;
    juce::ComboBox combo;
    juce::Label note;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

class HardwareChannelControl : public juce::Component
{
public:
    explicit HardwareChannelControl (BSIIControlAudioProcessor&);
    void resized() override;
    void refresh();
private:
    BSIIControlAudioProcessor& processor;
    juce::Label label, status;
    juce::ComboBox target;
    juce::TextButton apply { "Set Synth Channel" };
    int displayedChannel = -1;
    bool rejected = false;
};

class ParameterSection : public juce::Component
{
public:
    explicit ParameterSection (juce::String titleText);
    void addControl (juce::Component* control);
    void resized() override;
    void paint (juce::Graphics&) override;
    int preferredHeight (int width) const;
private:
    juce::String title;
    juce::Label globalNote;
    std::vector<juce::Component*> controls;
};

class EditorPage : public juce::Component
{
public:
    EditorPage();
    void addSection (ParameterSection* section);
    void resized() override;
private:
    juce::Component content;
    juce::Viewport viewport;
    std::vector<ParameterSection*> sections;
};

class BSIIControlAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit BSIIControlAudioProcessorEditor (BSIIControlAudioProcessor&);
    ~BSIIControlAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    // Selected-file send path; accepts only the validated temporary-patch command.
    bool sendEditBufferFile (const juce::File&);
    bool sendEditBufferPatch (const juce::MemoryBlock&);

private:
    void timerCallback() override;
    void refreshMidiOutputs();
    void autoSelectBassStation();
    void selectMidiOutput (int comboBoxId);
    bool reopenSelectedMidiOutput (bool forceReopen = false);
    void requestPatchFromHardware();
    void selectPreset (int program, bool sendToHardware);
    void forgetPreset (bool forgetProcessor = true);
    void sendAllStateToHardware();
    void saveSnapshot();
    void loadSnapshot();
    void armSysExCapture();
    void saveLastSysEx();
    void sendSysExFileToHardware();
    void importSysExToEditor();
    bool decodeSysExIntoEditor (const juce::MemoryBlock& data, const juce::String& sourceName, bool notifyHost = true);
    bool setEditorParameterValue (const char* parameterId, int value, bool notifyHost = true);
    void updateSysExDisplay();
    static juce::String hexPreview (const juce::MemoryBlock&, int maxBytes = 28);
    bool saveStateToFile (const juce::File& file);
    bool loadStateFromFile (const juce::File& file);
    int currentParameterValue (const char* parameterId) const;
    void applyHardwareUpdates();
    void updateRxDisplay();
    void updateFilterModeUi();
    void setLiveRxPaused (bool paused);
    void buildPages();

    BSIIControlAudioProcessor& processor;

    juce::Label titleLabel, subtitleLabel, statusLabel, rxLabel, nrpnLabel, sysexLabel, midiLabel, channelLabel, stateHintLabel, diagnosticLabel;
    juce::ComboBox midiOutputBox, channelBox, presetBox;
    juce::Label presetLabel, presetHint;
    juce::Label recallLabel, recallStatusLabel;
    juce::ComboBox recallModeBox;
    juce::TextButton captureProjectButton { "Capture Project Sound" };
    juce::TextButton previousPresetButton { "Previous" }, nextPresetButton { "Next" }, loadPresetButton { "Load Preset" };
    bsii::PresetSync presetSync;
    juce::TextButton refreshButton { "Refresh MIDI" };
    juce::ToggleButton pauseLiveRxButton { "Live RX is running" };
    juce::TextButton saveButton { juce::String::fromUTF8 ("Save Snapshot…") };
    juce::TextButton loadButton { juce::String::fromUTF8 ("Load Snapshot…") };
    juce::TextButton sendAllButton { juce::String::fromUTF8 ("Send All → BSII") };
    juce::TextButton getPatchButton { "Get Patch from BSII" };
    juce::TextButton armSysExButton { "Arm Patch Capture" };
    juce::TextButton saveSysExButton { juce::String::fromUTF8 ("Save Last .syx…") };
    juce::TextButton sendSysExButton { juce::String::fromUTF8 ("Send .syx → BSII") };
    juce::TextButton importSysExButton { juce::String::fromUTF8 ("Import .syx → Editor") };
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    std::unique_ptr<MicroTunePage> microTunePage;
    std::unique_ptr<BankPage> bankPage;
    std::unique_ptr<AfxPage> afxPage;
    std::unique_ptr<HardwareChannelControl> hardwareChannelControl;
    std::vector<std::unique_ptr<ParameterControl>> parameterControls;
    std::vector<std::unique_ptr<ChoiceControl>> choiceControls;
    std::vector<std::unique_ptr<ParameterSection>> sections;
    std::vector<std::unique_ptr<EditorPage>> pages;

    juce::Array<juce::MidiDeviceInfo> midiOutputs;
    std::unique_ptr<juce::MidiOutput>& midiOutput;
    juce::String selectedDeviceIdentifier;
    std::unique_ptr<juce::FileChooser> fileChooser;

    int midiChannel = 1;
    uint32_t lastDisplayedRxSerial = 0;
    uint32_t lastDisplayedNrpnSerial = 0;
    uint32_t lastDisplayedSysExSerial = 0;
    uint32_t captureArmSerial = 0;
    bool sysExCaptureArmed = false;
    bool patchRequestPending = false;
    uint32_t patchRequestStartSerial = 0;
    uint32_t patchRequestStartedMs = 0;
    uint32_t lastReconnectAttemptMs = 0;
    bool liveRxPaused = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BSIIControlAudioProcessorEditor)
};
