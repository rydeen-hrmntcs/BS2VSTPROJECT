#include "PluginEditor.h"
#include "BSIIMidi.h"
#include "BSIIPatchDecoder.h"
#include "BSIITuningDisplay.h"
#include "BSIIPatchState.h"
#include "BSIIProjectRecall.h"

ParameterControl::ParameterControl (juce::AudioProcessorValueTreeState& state,
                                    const bsii::ParameterSpec& parameterSpec,
                                    int specIndexIn)
    : index (specIndexIn), spec (parameterSpec)
{
    label.setText (spec.name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.86f));
    addAndMakeVisible (label);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 20);
    slider.setRange (spec.minimum, spec.maximum, 1.0);
    slider.setDoubleClickReturnValue (true, spec.defaultValue);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, spec.id, slider);
    const juce::String parameterId (spec.id);
    isOctaveSwitch = parameterId == "subOctaveRaw";
    isSyncSwitch = parameterId == "oscSyncRaw";
    if (isOctaveSwitch || isSyncSwitch)
    {
        firstChoice.setButtonText (isSyncSwitch ? "Off" : "-1");
        secondChoice.setButtonText (isSyncSwitch ? "On" : "-2");
        slider.setVisible (false);
        addAndMakeVisible (firstChoice);
        addAndMakeVisible (secondChoice);
        for (auto* button : { &firstChoice, &secondChoice })
        {
            button->setClickingTogglesState (true);
            button->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff087ea4));
            button->setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        }
        firstChoice.setConnectedEdges (juce::Button::ConnectedOnRight);
        secondChoice.setConnectedEdges (juce::Button::ConnectedOnLeft);
        // Keep raw storage compatible; each switch sends canonical endpoint values.
        firstChoice.onClick = [this] { slider.setValue (isSyncSwitch ? 0 : 127, juce::sendNotificationSync); updateDiscreteSwitch(); };
        secondChoice.onClick = [this] { slider.setValue (isSyncSwitch ? 127 : 0, juce::sendNotificationSync); updateDiscreteSwitch(); };
        updateDiscreteSwitch();
    }
    const bool fine = parameterId == "osc1Fine" || parameterId == "osc2Fine" || parameterId == "subFine";
    const bool coarse = parameterId == "osc1Coarse" || parameterId == "osc2Coarse" || parameterId == "subCoarse";
    if (fine || coarse)
    {
        // Set after attachment construction, which installs its own raw formatter.
        slider.textFromValueFunction = [fine] (double raw)
        {
            const auto value = fine ? bsii::fineDisplay (juce::roundToInt (raw))
                                    : bsii::coarseDisplay (juce::roundToInt (raw));
            return (value > 0 ? "+" : "") + juce::String (value, fine ? 0 : 1);
        };
        slider.valueFromTextFunction = [fine, parameterId] (const juce::String& text)
        {
            if (parameterId == "subFine" && text.getDoubleValue() == 0.0) return 127.0;
            return static_cast<double> (fine ? bsii::fineRaw (text.getDoubleValue())
                                            : bsii::coarseRaw (text.getDoubleValue()));
        };
        slider.setTooltip (fine ? "Fine tuning in cents (-100 to +100)."
                               : "Approximate coarse tuning in semitones (-12.0 to +12.0). Hardware display rounding may differ.");
        if (parameterId == "subFine" || parameterId == "subCoarse")
            slider.setTooltip ("Independent Sub tuning. Endpoints and zero are captured; intermediate displayed values are approximate.");
        slider.updateText();
    }
    if (parameterId == "inputGain")
    {
        slider.textFromValueFunction = [] (double raw) { return juce::String (juce::roundToInt (raw) * 2); };
        slider.valueFromTextFunction = [] (const juce::String& text)
        { return static_cast<double> (juce::jlimit (0, 31, juce::roundToInt (text.getDoubleValue() / 2.0))); };
        slider.setTooltip ("Global external input gain: 0 to 62 in steps of 2. Display units are not calibrated dB.");
        slider.updateText();
    }
    const bool signedAmount = parameterId == "velocityAmp" || parameterId == "velocityMod"
        || parameterId == "modWhLfo1Pitch" || parameterId == "modWhOsc2Pitch"
        || parameterId == "aftertouchFilter" || parameterId == "aftertouchLfo1"
        || parameterId == "aftertouchLfo2Spd" || parameterId == "modWhFilter"
        || parameterId == "modWhLfo2Filter";
    if (signedAmount)
    {
        slider.textFromValueFunction = [] (double raw)
        {
            const auto amount = juce::roundToInt (raw) - 64;
            return (amount > 0 ? "+" : "") + juce::String (amount);
        };
        slider.valueFromTextFunction = [this] (const juce::String& text)
        {
            return static_cast<double> (juce::jlimit (spec.minimum, spec.maximum, text.getIntValue() + 64));
        };
        slider.setTooltip ("Signed modulation amount (" + juce::String (spec.minimum - 64)
                           + " to +63). Zero is neutral; MIDI and snapshot storage remain unchanged.");
        slider.updateText();
    }
    if (parameterId == "bendAmount")
    {
        slider.textFromValueFunction = [] (double value)
        {
            const auto semitones = juce::roundToInt (value);
            return (semitones > 0 ? "+" : "") + juce::String (semitones);
        };
        slider.valueFromTextFunction = [] (const juce::String& text)
        {
            return static_cast<double> (text.getIntValue());
        };
        slider.setTooltip ("Pitch bend range in semitones (-24 to +24). Negative values reverse the wheel; zero disables bending.");
        slider.updateText();
    }
    slider.onValueChange = [this]
    {
        updateDiscreteSwitch();
    };
}

void ParameterControl::updateDiscreteSwitch()
{
    if (! isOctaveSwitch && ! isSyncSwitch) return;
    const bool firstSelected = isSyncSwitch ? slider.getValue() == 0 : slider.getValue() >= 63;
    firstChoice.setToggleState (firstSelected, juce::dontSendNotification);
    secondChoice.setToggleState (! firstSelected, juce::dontSendNotification);
}

void ParameterControl::resized()
{
    auto area = getLocalBounds();
    label.setBounds (area.removeFromTop (22));
    if (isOctaveSwitch || isSyncSwitch)
    {
        auto buttons = area.withSizeKeepingCentre (juce::jmin (132, area.getWidth() - 12), 38);
        firstChoice.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2));
        secondChoice.setBounds (buttons);
        return;
    }
    slider.setBounds (area);
}

void ParameterControl::setFromHardware (int value)
{
    slider.setValue (value, juce::dontSendNotification);
    updateDiscreteSwitch();
}

ChoiceControl::ChoiceControl (juce::AudioProcessorValueTreeState& state,
                              const bsii::ChoiceSpec& choiceSpec,
                              int choiceIndexIn)
    : index (choiceIndexIn), spec (choiceSpec)
{
    label.setText (spec.name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);

    for (int i = 0; i < static_cast<int> (spec.choices.size()); ++i)
        combo.addItem (spec.choices[static_cast<size_t> (i)].label, i + 1);
    addAndMakeVisible (combo);

    const juce::String parameterId (spec.id);
    note.setText ((parameterId == "ampTrigger" || parameterId == "modTrigger")
                      ? "Shared: Amp + Mod"
                      : (spec.verifiedWireValues ? "MIDI verified" : "MIDI test"),
                  juce::dontSendNotification);
    note.setJustificationType (juce::Justification::centred);
    note.setColour (juce::Label::textColourId,
                    spec.verifiedWireValues ? juce::Colours::lightgreen.withAlpha (0.75f)
                                            : juce::Colours::orange.withAlpha (0.85f));
    addAndMakeVisible (note);

    if (parameterId == "arpMode")
        combo.setTooltip ("Record arms hardware sequence recording: incoming notes can replace sequence data. Send All will not arm Record; select it explicitly here or on the hardware.");
    if (parameterId == "arpOctaves")
        combo.setTooltip ("Arpeggiator octave span (1-4). The hardware shares this selector with SEQ in Play/Record; sequence-slot playback contents have not been verified.");
    if (parameterId == "lfo1Sync" || parameterId == "lfo2Sync")
        combo.setTooltip ("Speed uses the normal LFO Speed value; Sync uses Division and the synth's internal or external MIDI clock.");
    if (parameterId == "lfo1Division" || parameterId == "lfo2Division")
        combo.setTooltip ("Clock division used in Sync mode. Beat values use quarter-note beats; the normal Speed setting is retained separately.");

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, spec.id, combo);
}

void ChoiceControl::resized()
{
    auto area = getLocalBounds().reduced (6);
    label.setBounds (area.removeFromTop (24));
    area.removeFromTop (18);
    combo.setBounds (area.removeFromTop (28));
    note.setBounds (area.removeFromTop (22));
}

bool ChoiceControl::setFromWireValue (int wireValue)
{
    for (int i = 0; i < static_cast<int> (spec.choices.size()); ++i)
    {
        if (spec.choices[static_cast<size_t> (i)].wireValue == wireValue)
        {
            combo.setSelectedId (i + 1, juce::dontSendNotification);
            return true;
        }
    }
    note.setText ("RX raw " + juce::String (wireValue), juce::dontSendNotification);
    note.setColour (juce::Label::textColourId, juce::Colours::orange.withAlpha (0.9f));
    return false;
}

void ChoiceControl::setInteractionEnabled (bool enabled)
{
    combo.setEnabled (enabled);
    note.setAlpha (enabled ? 1.0f : 0.55f);
}

HardwareChannelControl::HardwareChannelControl (BSIIControlAudioProcessor& p) : processor (p)
{
    label.setText ("Hardware MIDI Channel", juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    for (int i = 1; i <= 16; ++i) target.addItem (juce::String (i), i);
    addAndMakeVisible (label); addAndMakeVisible (target);
    addAndMakeVisible (apply); addAndMakeVisible (status);
    status.setJustificationType (juce::Justification::topLeft);
    status.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.75f));
    apply.setTooltip ("Changes the synth channel. The plugin follows confirmation; update DAW note routing separately.");
    target.onChange = [this] { refresh(); };
    apply.onClick = [this] { rejected = ! processor.requestHardwareMidiChannel (target.getSelectedId()); refresh(); };
    refresh();
}

void HardwareChannelControl::resized()
{
    auto area = getLocalBounds().reduced (6);
    label.setBounds (area.removeFromTop (22));
    auto row = area.removeFromTop (32);
    target.setBounds (row.removeFromLeft (60)); row.removeFromLeft (6);
    apply.setBounds (row);
    area.removeFromTop (4); status.setBounds (area);
}

void HardwareChannelControl::refresh()
{
    const auto channel = processor.getHardwareMidiChannel();
    if (displayedChannel != channel)
    {
        displayedChannel = channel;
        target.setSelectedId (channel, juce::dontSendNotification);
    }
    const auto info = processor.getHardwareChannelInfo();
    target.setEnabled (info.pending < 0);
    apply.setEnabled (processor.hardwareMidiOutput() != nullptr && info.pending < 0 && target.getSelectedId() != channel);
    juce::String text;
    if (info.pending > 0) text = "Waiting for synth channel " + juce::String (info.pending) + "...";
    else if (info.timedOut) text = "No confirmation. Check the synth and the plugin Channel selector.";
    else if (rejected) text = "Connect the synth and finish any capture or recall first.";
    else if (info.reported == channel) text = "Synth confirmed channel " + juce::String (channel) + ". Match DAW note routing.";
    else text = "Plugin uses channel " + juce::String (channel) + ". Select a new synth channel.";
    status.setText (text, juce::dontSendNotification);
}

ParameterSection::ParameterSection (juce::String titleText) : title (std::move (titleText))
{
    if (title == "Global")
    {
        globalNote.setText ("Affects the synth across presets. Shown value follows edits and live MIDI; Get Patch does not read Input Gain.",
                            juce::dontSendNotification);
        globalNote.setJustificationType (juce::Justification::topLeft);
        globalNote.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.75f));
        addAndMakeVisible (globalNote);
    }
}
void ParameterSection::addControl (juce::Component* control) { controls.push_back (control); addAndMakeVisible (control); }

int ParameterSection::preferredHeight (int width) const
{
    const int controlsPerRow = juce::jlimit (1, 6, (width - 20) / 120);
    const auto rows = juce::jmax (1, static_cast<int> ((controls.size() + controlsPerRow - 1) / controlsPerRow));
    return 38 + rows * 126 + 12 + (title == "Global" ? 48 : 0);
}

void ParameterSection::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (juce::FontOptions (16.0f).withStyle ("Bold"));
    g.drawText (title, 14, 6, getWidth() - 28, 24, juce::Justification::centredLeft);
}

void ParameterSection::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (28);
    if (title == "Global") globalNote.setBounds (area.removeFromTop (48));
    const int controlsPerRow = juce::jlimit (1, 6, area.getWidth() / 120);
    constexpr int controlHeight = 126;

    for (size_t i = 0; i < controls.size(); ++i)
    {
        const int row = static_cast<int> (i) / controlsPerRow;
        const int column = static_cast<int> (i) % controlsPerRow;
        const int columnsThisRow = juce::jmin (controlsPerRow,
            static_cast<int> (controls.size()) - row * controlsPerRow);
        const int cellWidth = area.getWidth() / juce::jmax (1, columnsThisRow);
        controls[i]->setBounds (area.getX() + column * cellWidth,
                                area.getY() + row * controlHeight,
                                cellWidth, controlHeight);
    }
}

EditorPage::EditorPage()
{
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);
}
void EditorPage::addSection (ParameterSection* section) { sections.push_back (section); content.addAndMakeVisible (section); }
void EditorPage::resized()
{
    viewport.setBounds (getLocalBounds());
    const int width = juce::jmax (1, getWidth() - viewport.getScrollBarThickness());
    int totalHeight = 16;
    for (auto* section : sections)
        totalHeight += section->preferredHeight (juce::jmax (1, width - 16)) + 8;
    content.setSize (width, juce::jmax (getHeight(), totalHeight));
    auto area = content.getLocalBounds().reduced (8);
    for (auto* section : sections)
    {
        const auto h = section->preferredHeight (area.getWidth());
        section->setBounds (area.removeFromTop (h));
        area.removeFromTop (8);
    }
}

BSIIControlAudioProcessorEditor::BSIIControlAudioProcessorEditor (BSIIControlAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), midiOutput (p.hardwareMidiOutput())
{
    setResizable (true, true);
    setResizeLimits (760, 600, 1600, 1200);
    setSize (900, 800);
    titleLabel.setText (juce::String::fromUTF8 ("Bass Station II Control — Waveforms + SysEx Validation v0.8.2"), juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (23.0f).withStyle ("Bold"));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Oscillator waveforms are now exposed via live MIDI. Pause Live RX to validate SysEx separately from CC/NRPN sync.", juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.68f));
    addAndMakeVisible (subtitleLabel);

    midiLabel.setText ("MIDI output", juce::dontSendNotification);
    channelLabel.setText ("Channel", juce::dontSendNotification);
    addAndMakeVisible (midiLabel);
    addAndMakeVisible (channelLabel);
    addAndMakeVisible (midiOutputBox);
    addAndMakeVisible (channelBox);
    addAndMakeVisible (refreshButton);
    addAndMakeVisible (pauseLiveRxButton);

    for (int channel = 1; channel <= 16; ++channel)
        channelBox.addItem (juce::String (channel), channel);
    midiChannel = processor.getHardwareMidiChannel();
    channelBox.setSelectedId (midiChannel, juce::dontSendNotification);
    channelBox.onChange = [this]
    {
        midiChannel = juce::jmax (1, channelBox.getSelectedId());
        processor.setHardwareMidiChannel (midiChannel);
        forgetPreset();
    };

    refreshButton.onClick = [this] { refreshMidiOutputs(); autoSelectBassStation(); };
    pauseLiveRxButton.setToggleState (! processor.isLiveMidiRxEnabled(), juce::dontSendNotification);
    setLiveRxPaused (! processor.isLiveMidiRxEnabled());
    pauseLiveRxButton.onClick = [this] { setLiveRxPaused (pauseLiveRxButton.getToggleState()); };
    midiOutputBox.onChange = [this] { selectMidiOutput (midiOutputBox.getSelectedId()); };

    presetLabel.setText ("Preset", juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected ("Unknown / edit buffer");
    presetBox.setTooltip ("Selecting a preset loads it on the Bass Station II, replacing the temporary sound. Presets are numbered 0 to 127.");
    for (int program = 0; program < 128; ++program)
        presetBox.addItem (juce::String (program), program + 1);
    presetHint.setText ("Replaces temporary sound.", juce::dontSendNotification);
    for (auto* component : std::initializer_list<juce::Component*> { &presetLabel, &presetBox,
             &previousPresetButton, &nextPresetButton, &loadPresetButton, &presetHint })
        addAndMakeVisible (component);
    presetBox.onChange = [this] { selectPreset (presetBox.getSelectedId() - 1, true); };
    previousPresetButton.onClick = [this] { selectPreset (presetSync.selected() - 1, true); };
    nextPresetButton.onClick = [this] { selectPreset (presetSync.selected() + 1, true); };
    loadPresetButton.onClick = [this] { selectPreset (presetBox.getSelectedId() - 1, true); };
    forgetPreset (false);

    recallLabel.setText ("On reconnect", juce::dontSendNotification);
    recallModeBox.addItem ("Off", 1);
    recallModeBox.addItem ("Stored preset", 2);
    recallModeBox.addItem ("Captured sound", 3);
    recallModeBox.setSelectedId (static_cast<int> (processor.getProjectRecallInfo().mode) + 1, juce::dontSendNotification);
    recallModeBox.setTooltip ("Once when loading this saved Set or reconnecting USB. Does not run on every Play. Stored preset uses the selected preset number; captured sound restores only the edit buffer.");
    recallModeBox.onChange = [this]
    {
        processor.configureProjectRecall (static_cast<BSIIControlAudioProcessor::RecallMode> (recallModeBox.getSelectedId() - 1),
                                          processor.getKnownHardwareProgram());
        recallModeBox.setSelectedId (static_cast<int> (processor.getProjectRecallInfo().mode) + 1, juce::dontSendNotification);
    };
    captureProjectButton.setTooltip ("Read a fresh copy of the hardware edit buffer into this project. Choose Captured sound and save the Set to enable recall. No stored preset is overwritten.");
    captureProjectButton.onClick = [this]
    {
        if (patchRequestPending || presetSync.pending())
        { statusLabel.setText ("Wait for the preset/patch refresh before capturing.", juce::dontSendNotification); return; }
        processor.captureProjectSound();
    };
    for (auto* component : std::initializer_list<juce::Component*> { &recallLabel, &recallModeBox, &captureProjectButton, &recallStatusLabel })
        addAndMakeVisible (component);

    saveButton.onClick = [this] { saveSnapshot(); };
    loadButton.onClick = [this] { loadSnapshot(); };
    sendAllButton.onClick = [this] { sendAllStateToHardware(); };
    getPatchButton.onClick = [this] { requestPatchFromHardware(); };
    armSysExButton.onClick = [this] { armSysExCapture(); };
    saveSysExButton.onClick = [this] { saveLastSysEx(); };
    sendSysExButton.onClick = [this] { sendSysExFileToHardware(); };
    importSysExButton.onClick = [this] { importSysExToEditor(); };
    addAndMakeVisible (saveButton);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (sendAllButton);
    addAndMakeVisible (getPatchButton);
    addAndMakeVisible (armSysExButton);
    addAndMakeVisible (saveSysExButton);
    addAndMakeVisible (sendSysExButton);
    addAndMakeVisible (importSysExButton);

    stateHintLabel.setText ("Get Patch requests the current edit-buffer patch automatically. Manual Function/Exit + Dump capture remains available as a fallback.",
                            juce::dontSendNotification);
    stateHintLabel.setColour (juce::Label::textColourId, juce::Colours::orange.withAlpha (0.82f));
    stateHintLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (stateHintLabel);

    addAndMakeVisible (tabs);
    addAndMakeVisible (rxLabel);
    addAndMakeVisible (nrpnLabel);
    addAndMakeVisible (sysexLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (diagnosticLabel);
    rxLabel.setText ("CC RX: waiting...", juce::dontSendNotification);
    nrpnLabel.setText ("NRPN RX: waiting...", juce::dontSendNotification);
    sysexLabel.setText ("SysEx RX: waiting...", juce::dontSendNotification);
    rxLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.78f));
    nrpnLabel.setColour (juce::Label::textColourId, juce::Colours::lightblue.withAlpha (0.88f));
    sysexLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen.withAlpha (0.88f));
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.75f));
    diagnosticLabel.setColour (juce::Label::textColourId, juce::Colours::yellow.withAlpha (0.82f));
    diagnosticLabel.setJustificationType (juce::Justification::centredLeft);
    diagnosticLabel.setText ("SysEx validation: no patch decoded yet.", juce::dontSendNotification);

    buildPages();
    refreshMidiOutputs();
    autoSelectBassStation();
    if (processor.getKnownHardwareProgram() >= 0) selectPreset (processor.getKnownHardwareProgram(), false);
    startTimerHz (30);
}

BSIIControlAudioProcessorEditor::~BSIIControlAudioProcessorEditor()
{
    stopTimer();
    fileChooser.reset();
}

void BSIIControlAudioProcessorEditor::buildPages()
{
    const std::vector<std::pair<juce::String, std::vector<juce::String>>> pageDefinitions
    {
        { "Osc 1", { "Osc 1" } },
        { "Osc 2", { "Osc 2" } },
        { "Mixer", { "Mixer" } },
        { "Filter", { "Filter" } },
        { "LFO", { "LFO / Glide" } },
        { "Amp Env", { "Amp Env" } },
        { "Mod Env", { "Mod Env" } },
        { "FX + Perf", { "FX / Perf" } },
        { "Arp / Seq", { "Arp / Seq" } },
        { "Global", { "Global" } }
    };

    const auto& params = bsii::parameterSpecs();
    const auto& choices = bsii::choiceSpecs();

    for (const auto& pageDef : pageDefinitions)
    {
        auto page = std::make_unique<EditorPage>();
        for (const auto& sectionName : pageDef.second)
        {
            auto section = std::make_unique<ParameterSection> (sectionName);

            for (size_t i = 0; i < choices.size(); ++i)
            {
                if (sectionName != choices[i].section) continue;
                auto control = std::make_unique<ChoiceControl> (processor.getParameters(), choices[i], static_cast<int> (i));
                section->addControl (control.get());
                choiceControls.push_back (std::move (control));
            }

            for (size_t i = 0; i < params.size(); ++i)
            {
                if (sectionName != params[i].section
                    && ! (sectionName == "Osc 1" && juce::String (params[i].section) == "Master")) continue;
                auto control = std::make_unique<ParameterControl> (processor.getParameters(), params[i], static_cast<int> (i));
                section->addControl (control.get());
                parameterControls.push_back (std::move (control));
            }

            if (sectionName == "Global")
            {
                hardwareChannelControl = std::make_unique<HardwareChannelControl> (processor);
                section->addControl (hardwareChannelControl.get());
            }
            page->addSection (section.get());
            sections.push_back (std::move (section));
        }
        tabs.addTab (pageDef.first, juce::Colours::transparentBlack, page.get(), false);
        pages.push_back (std::move (page));
    }
    microTunePage = std::make_unique<MicroTunePage> (processor);
    tabs.addTab ("MicroTune", juce::Colours::transparentBlack, microTunePage.get(), false);
    bankPage = std::make_unique<BankPage> (processor, [this](const juce::MemoryBlock& patch)
    {
        if (!sendEditBufferPatch(patch)) return false;
        decodeSysExIntoEditor(patch,"selected bank entry",false);
        return true;
    }, [this]{ return !patchRequestPending && !presetSync.pending() && !sysExCaptureArmed; });
    tabs.addTab ("Banks", juce::Colours::transparentBlack, bankPage.get(), false);
    afxPage=std::make_unique<AfxPage>(processor);
    tabs.addTab("AFX",juce::Colours::transparentBlack,afxPage.get(),false);
}

void BSIIControlAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15171a));
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (10.0f), 8.0f, 1.0f);
}

void BSIIControlAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    titleLabel.setBounds (area.removeFromTop (34));
    subtitleLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (6);

    auto deviceRow = area.removeFromTop (32);
    midiLabel.setBounds (deviceRow.removeFromLeft (82));
    midiOutputBox.setBounds (deviceRow.removeFromLeft (juce::jlimit (140, 280, area.getWidth() - 552)));
    deviceRow.removeFromLeft (12);
    channelLabel.setBounds (deviceRow.removeFromLeft (58));
    channelBox.setBounds (deviceRow.removeFromLeft (70));
    deviceRow.removeFromLeft (10);
    refreshButton.setBounds (deviceRow.removeFromLeft (120));
    deviceRow.removeFromLeft (10);
    pauseLiveRxButton.setBounds (deviceRow.removeFromLeft (190));

    area.removeFromTop (8);
    auto presetRow = area.removeFromTop (32);
    presetLabel.setBounds (presetRow.removeFromLeft (82));
    presetBox.setBounds (presetRow.removeFromLeft (170));
    presetRow.removeFromLeft (8);
    previousPresetButton.setBounds (presetRow.removeFromLeft (85));
    presetRow.removeFromLeft (6);
    nextPresetButton.setBounds (presetRow.removeFromLeft (65));
    presetRow.removeFromLeft (6);
    loadPresetButton.setBounds (presetRow.removeFromLeft (110));
    presetRow.removeFromLeft (8);
    presetHint.setBounds (presetRow);
    area.removeFromTop (8);
    auto recallRow = area.removeFromTop (32);
    recallLabel.setBounds (recallRow.removeFromLeft (100));
    recallModeBox.setBounds (recallRow.removeFromLeft (160));
    recallRow.removeFromLeft (8);
    captureProjectButton.setBounds (recallRow.removeFromLeft (180));
    recallRow.removeFromLeft (8);
    recallStatusLabel.setBounds (recallRow);
    area.removeFromTop (8);
    auto stateRow = area.removeFromTop (32);
    saveButton.setBounds (stateRow.removeFromLeft (150));
    stateRow.removeFromLeft (8);
    loadButton.setBounds (stateRow.removeFromLeft (150));
    stateRow.removeFromLeft (8);
    sendAllButton.setBounds (stateRow.removeFromLeft (170));
    stateRow.removeFromLeft (8);
    getPatchButton.setBounds (stateRow.removeFromLeft (190));

    area.removeFromTop (6);
    auto sysexRow = area.removeFromTop (32);
    armSysExButton.setBounds (sysexRow.removeFromLeft (160));
    sysexRow.removeFromLeft (8);
    saveSysExButton.setBounds (sysexRow.removeFromLeft (150));
    sysexRow.removeFromLeft (8);
    sendSysExButton.setBounds (sysexRow.removeFromLeft (160));
    sysexRow.removeFromLeft (8);
    importSysExButton.setBounds (sysexRow.removeFromLeft (170));
    sysexRow.removeFromLeft (14);
    stateHintLabel.setBounds (sysexRow);

    area.removeFromTop (10);
    auto bottom = area.removeFromBottom (136);
    rxLabel.setBounds (bottom.removeFromTop (24));
    nrpnLabel.setBounds (bottom.removeFromTop (24));
    sysexLabel.setBounds (bottom.removeFromTop (24));
    statusLabel.setBounds (bottom.removeFromTop (28));
    diagnosticLabel.setBounds (bottom.removeFromTop (28));
    area.removeFromBottom (6);
    tabs.setBounds (area);
}

void BSIIControlAudioProcessorEditor::timerCallback()
{
    const auto devicesNow = juce::MidiOutput::getAvailableDevices();
    if (devicesNow != midiOutputs)
    {
        const auto previousId = selectedDeviceIdentifier;
        refreshMidiOutputs();
        for (int i = 0; i < midiOutputs.size(); ++i)
            if (midiOutputs[i].identifier == previousId)
            {
                midiOutputBox.setSelectedId (i + 1, juce::sendNotificationSync);
                break;
            }
        if (midiOutputBox.getSelectedId() == 0)
            autoSelectBassStation();
    }

    if (midiChannel != processor.getHardwareMidiChannel())
    {
        midiChannel = processor.getHardwareMidiChannel();
        channelBox.setSelectedId (midiChannel, juce::dontSendNotification);
        forgetPreset();
    }
    if (liveRxPaused == processor.isLiveMidiRxEnabled())
        setLiveRxPaused (! processor.isLiveMidiRxEnabled());
    if (midiOutput != nullptr && selectedDeviceIdentifier != midiOutput->getIdentifier())
    {
        forgetPreset();
        autoSelectBassStation();
    }

    const auto nowMs = juce::Time::getMillisecondCounter();
    if (midiOutput == nullptr && selectedDeviceIdentifier.isNotEmpty()
        && nowMs - lastReconnectAttemptMs > 2000u)
    {
        lastReconnectAttemptMs = nowMs;
        reopenSelectedMidiOutput (false);
    }

    if (patchRequestPending && nowMs - patchRequestStartedMs > 2500u)
    {
        patchRequestPending = false;
        statusLabel.setText (processor.usesDirectHardwareSysEx()
            ? "No patch reply received. Check the synth USB connection, click Refresh MIDI, then try Get Patch again."
            : "No patch reply received. In Standalone Settings, enable Bass Station II under Active MIDI inputs, then try Get Patch again.",
            juce::dontSendNotification);
    }

    const bool storedBusy=processor.isStoredPatchBusy();
    const auto channelChange = processor.getHardwareChannelInfo();
    channelBox.setEnabled (channelChange.pending < 0 && !storedBusy);
    midiOutputBox.setEnabled (channelChange.pending < 0 && !storedBusy);
    refreshButton.setEnabled (channelChange.pending < 0 && !storedBusy);
    sendAllButton.setEnabled (channelChange.pending < 0 && !storedBusy);
    const auto recall = processor.getProjectRecallInfo();
    recallStatusLabel.setText (recall.status, juce::dontSendNotification);
    recallStatusLabel.setTooltip (recall.status);
    recallModeBox.setSelectedId (static_cast<int> (recall.mode) + 1, juce::dontSendNotification);
    captureProjectButton.setEnabled (!storedBusy && ! recall.capturing && ! patchRequestPending && ! presetSync.pending());
    recallModeBox.setEnabled (!storedBusy && ! recall.capturing);
    presetBox.setEnabled (!storedBusy && ! recall.capturing);
    getPatchButton.setEnabled (!storedBusy && ! recall.capturing);

    const auto program = processor.consumeHardwareProgram();
    if (program >= 0) selectPreset (program, false);
    applyHardwareUpdates();
    if (hardwareChannelControl) hardwareChannelControl->refresh();
    updateRxDisplay();
    updateSysExDisplay();
    updateFilterModeUi();
    if (presetSync.ready (juce::Time::getMillisecondCounter()) && ! patchRequestPending && midiOutput != nullptr)
        requestPatchFromHardware();
}

void BSIIControlAudioProcessorEditor::refreshMidiOutputs()
{
    midiOutputs = juce::MidiOutput::getAvailableDevices();
    midiOutputBox.clear (juce::dontSendNotification);
    for (int i = 0; i < midiOutputs.size(); ++i)
        midiOutputBox.addItem (midiOutputs[i].name, i + 1);
    statusLabel.setText (midiOutputs.isEmpty() ? "No MIDI outputs detected." : "Select the Bass Station II MIDI output.",
                         juce::dontSendNotification);
}

void BSIIControlAudioProcessorEditor::autoSelectBassStation()
{
    if (midiOutput != nullptr)
        for (int i = 0; i < midiOutputs.size(); ++i)
            if (midiOutputs[i].identifier == midiOutput->getIdentifier())
            {
                selectedDeviceIdentifier = midiOutputs[i].identifier;
                midiOutputBox.setSelectedId (i + 1, juce::dontSendNotification);
                return;
            }
    if (processor.hasSavedHardwareConnection())
    {
        statusLabel.setText ("Saved MIDI output is reconnecting. If unavailable, connect the hardware or select an output.",
                             juce::dontSendNotification);
        return;
    }
    for (int i = 0; i < midiOutputs.size(); ++i)
    {
        const auto name = midiOutputs[i].name.toLowerCase();
        if (name.contains ("bass station") || name.contains ("bassstation"))
        {
            midiOutputBox.setSelectedId (i + 1, juce::sendNotificationSync);
            return;
        }
    }
}

void BSIIControlAudioProcessorEditor::selectMidiOutput (int comboBoxId)
{
    const int index = comboBoxId - 1;
    if (! juce::isPositiveAndBelow (index, midiOutputs.size()))
    {
        forgetPreset();
        processor.clearPendingMidi();
        midiOutput.reset();
        selectedDeviceIdentifier.clear();
        processor.rememberHardwareMidiOutput ({}, {});
        processor.connectHardwareSysExInput ({});
        statusLabel.setText ("No MIDI output selected.", juce::dontSendNotification);
        return;
    }

    if (selectedDeviceIdentifier != midiOutputs[index].identifier) forgetPreset();
    selectedDeviceIdentifier = midiOutputs[index].identifier;
    reopenSelectedMidiOutput (true);
}

bool BSIIControlAudioProcessorEditor::reopenSelectedMidiOutput (bool forceReopen)
{
    if (selectedDeviceIdentifier.isEmpty())
        return false;

    if (midiOutput != nullptr && ! forceReopen)
        return true;

    auto devices = juce::MidiOutput::getAvailableDevices();
    juce::String deviceName;
    bool found = false;
    for (const auto& device : devices)
    {
        if (device.identifier == selectedDeviceIdentifier)
        {
            deviceName = device.name;
            found = true;
            break;
        }
    }

    if (! found)
    {
        processor.connectHardwareSysExInput ({});
        midiOutput.reset();
        return false;
    }

    processor.clearPendingMidi();
    midiOutput.reset();
    midiOutput = juce::MidiOutput::openDevice (selectedDeviceIdentifier);
    if (midiOutput != nullptr)
    {
        processor.rememberHardwareMidiOutput (selectedDeviceIdentifier, deviceName);
        const bool inputReady = processor.connectHardwareSysExInput (deviceName);
        const auto receiveHint = processor.usesDirectHardwareSysEx()
            ? (inputReady ? ". Patch and live control RX connected directly."
                          : ". Direct RX unavailable: no unique matching MIDI input. Live controls use the DAW input.")
            : ". RX requires Bass Station II enabled under Settings -> Active MIDI inputs.";
        statusLabel.setText ("TX connected to " + deviceName + receiveHint, juce::dontSendNotification);
        return true;
    }

    processor.connectHardwareSysExInput ({});
    statusLabel.setText ("Could not open " + deviceName + ".", juce::dontSendNotification);
    return false;
}

void BSIIControlAudioProcessorEditor::forgetPreset (bool forgetProcessor)
{
    if (forgetProcessor) processor.rememberHardwareProgram (-1);
    presetSync.forget();
    presetBox.setSelectedId (0, juce::dontSendNotification);
    previousPresetButton.setEnabled (false);
    nextPresetButton.setEnabled (false);
    loadPresetButton.setEnabled (false);
}

void BSIIControlAudioProcessorEditor::selectPreset (int program, bool sendToHardware)
{
    if(sendToHardware && processor.isStoredPatchBusy()) return;
    if (program < 0 || program > 127 || processor.getProjectRecallInfo().capturing) return;
    if (sendToHardware)
    {
        if (! reopenSelectedMidiOutput()) return;
        midiOutput->sendMessageNow (juce::MidiMessage::programChange (midiChannel, program));
    }
    processor.rememberHardwareProgram (program);
    if (sendToHardware && processor.getProjectRecallInfo().mode == BSIIControlAudioProcessor::RecallMode::preset)
        processor.configureProjectRecall (BSIIControlAudioProcessor::RecallMode::preset, program);
    presetSync.select (program, juce::Time::getMillisecondCounter());
    presetBox.setSelectedId (program + 1, juce::dontSendNotification);
    previousPresetButton.setEnabled (program > 0);
    nextPresetButton.setEnabled (program < 127);
    loadPresetButton.setEnabled (true);
    statusLabel.setText ("Preset " + juce::String (program) + juce::String::fromUTF8 (": waiting to refresh patch…"), juce::dontSendNotification);
}

void BSIIControlAudioProcessorEditor::requestPatchFromHardware()
{
    if(processor.isStoredPatchBusy()) { statusLabel.setText("Wait for the stored-preset operation to finish.",juce::dontSendNotification); return; }
    if (patchRequestPending || processor.getProjectRecallInfo().capturing) return;
    if (! reopenSelectedMidiOutput (true))
    {
        statusLabel.setText ("Cannot request patch: Bass Station II MIDI output is not open.", juce::dontSendNotification);
        return;
    }

    if (processor.usesDirectHardwareSysEx()
        && ! processor.connectHardwareSysExInput (midiOutput->getName()))
    {
        statusLabel.setText ("Cannot request patch: no unique matching hardware MIDI input. Check USB and click Refresh MIDI.",
                             juce::dontSendNotification);
        return;
    }

    const auto latest = processor.getLatestSysEx();
    patchRequestStartSerial = latest.serial;
    presetSync.requestStarted();
    patchRequestPending = true;
    patchRequestStartedMs = juce::Time::getMillisecondCounter();

    static constexpr uint8_t requestBytes[] { 0xF0, 0x00, 0x20, 0x29, 0x00, 0x33, 0x00, 0x40, 0xF7 };
    juce::MidiMessage request (requestBytes, static_cast<int> (sizeof (requestBytes)), 0.0);
    midiOutput->sendMessageNow (request);
    statusLabel.setText (juce::String::fromUTF8 ("Requested current patch from Bass Station II…"), juce::dontSendNotification);
}

int BSIIControlAudioProcessorEditor::currentParameterValue (const char* parameterId) const
{
    if (auto* value = processor.getParameters().getRawParameterValue (parameterId))
        return juce::roundToInt (value->load());
    return 0;
}

void BSIIControlAudioProcessorEditor::sendAllStateToHardware()
{
    if(processor.isStoredPatchBusy()) return;
    if (midiOutput == nullptr)
    {
        statusLabel.setText ("Cannot send state: no MIDI output is open.", juce::dontSendNotification);
        return;
    }

    processor.clearPendingMidi();
    const auto& params = bsii::parameterSpecs();
    const auto& choices = bsii::choiceSpecs();

    for (size_t i = 0; i < params.size(); ++i)
    {
        if (! bsii::isPatchParameter (params[i].id)) continue;
        const auto value = currentParameterValue (params[i].id);
        bsii::sendParameter (*midiOutput, midiChannel, params[i], value);
    }

    for (size_t i = 0; i < choices.size(); ++i)
    {
        const auto choiceIndex = juce::jlimit (0, static_cast<int> (choices[i].choices.size()) - 1,
                                              currentParameterValue (choices[i].id));
        // Bulk sound recall must not arm recording of incoming sequence notes.
        if (juce::String (choices[i].id) == "arpMode" && choiceIndex == 7) continue;
        bsii::sendChoice (*midiOutput, midiChannel, choices[i],
                          choices[i].choices[static_cast<size_t> (choiceIndex)].wireValue);
    }

    statusLabel.setText ("Sent editor state (Send All never arms Record) on MIDI channel " + juce::String (midiChannel) + ".",
                         juce::dontSendNotification);
}

juce::String BSIIControlAudioProcessorEditor::hexPreview (const juce::MemoryBlock& block, int maxBytes)
{
    const auto* bytes = static_cast<const uint8_t*> (block.getData());
    const auto count = juce::jmin (static_cast<int> (block.getSize()), maxBytes);
    juce::StringArray parts;
    for (int i = 0; i < count; ++i)
        parts.add (juce::String::toHexString (static_cast<int> (bytes[i])).paddedLeft ('0', 2).toUpperCase());
    auto result = parts.joinIntoString (" ");
    if (static_cast<int> (block.getSize()) > count) result += juce::String::fromUTF8 (" …");
    return result;
}

void BSIIControlAudioProcessorEditor::armSysExCapture()
{
    const auto latest = processor.getLatestSysEx();
    captureArmSerial = latest.serial;
    sysExCaptureArmed = true;
    statusLabel.setText ("Armed. On BSII: hold Function/Exit + top C (Dump) until onE, then press Dump again while still holding Function.",
                         juce::dontSendNotification);
}

void BSIIControlAudioProcessorEditor::updateSysExDisplay()
{
    const auto latest = processor.getLatestSysEx();
    if (latest.serial == 0 || latest.serial == lastDisplayedSysExSerial)
        return;

    lastDisplayedSysExSerial = latest.serial;
    const auto size = static_cast<int> (latest.data.getSize());
    const auto preview = hexPreview (latest.data);
    sysexLabel.setText ("SysEx RX: " + juce::String (size) + " bytes | " + preview,
                        juce::dontSendNotification);

    if (patchRequestPending && latest.serial > patchRequestStartSerial && bsii::isRecallPatch(latest.data))
    {
        patchRequestPending = false;
        if (! presetSync.responseCurrent()) return;
        if (decodeSysExIntoEditor (latest.data, "live Bass Station II patch", false))
            statusLabel.setText ("Live patch received and decoded from Bass Station II (" + juce::String (size) + " bytes).",
                                 juce::dontSendNotification);
        return;
    }

    if (sysExCaptureArmed && latest.serial > captureArmSerial)
    {
        sysExCaptureArmed = false;
        const auto* bytes = static_cast<const uint8_t*> (latest.data.getData());
        const bool looksLikeNovation = latest.data.getSize() >= 5
            && bytes[0] == 0xF0 && bytes[1] == 0x00 && bytes[2] == 0x20 && bytes[3] == 0x29;
        statusLabel.setText (looksLikeNovation
            ? "Captured a Novation SysEx message (" + juce::String (size) + juce::String::fromUTF8 (" bytes). Use Save Last .syx… to keep the raw patch dump.")
            : "Captured SysEx (" + juce::String (size) + " bytes). Header is not the expected Novation manufacturer ID.",
            juce::dontSendNotification);
    }
}

void BSIIControlAudioProcessorEditor::saveLastSysEx()
{
    const auto latest = processor.getLatestSysEx();
    if (latest.data.isEmpty())
    {
        statusLabel.setText ("No SysEx has been received yet.", juce::dontSendNotification);
        return;
    }

    auto defaultFile = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                           .getChildFile ("Bass Station II Patch.syx");
    fileChooser = std::make_unique<juce::FileChooser> ("Save captured Bass Station II SysEx", defaultFile, "*.syx");
    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync (flags, [this, data = latest.data] (const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file != juce::File{})
        {
            if (! file.hasFileExtension ("syx")) file = file.withFileExtension ("syx");
            const bool ok = file.replaceWithData (data.getData(), data.getSize());
            statusLabel.setText (ok ? "Saved raw SysEx: " + file.getFileName() : "Could not save SysEx file.",
                                 juce::dontSendNotification);
        }
        fileChooser.reset();
    });
}

bool BSIIControlAudioProcessorEditor::sendEditBufferFile (const juce::File& file)
{
    if (midiOutput == nullptr)
    {
        statusLabel.setText ("Cannot send patch: no MIDI output is open.", juce::dontSendNotification);
        return false;
    }
    juce::FileInputStream input (file);
    if (! input.openedOk())
    {
        statusLabel.setText ("Could not read patch file.", juce::dontSendNotification);
        return false;
    }
    juce::MemoryBlock data;
    input.readIntoMemoryBlock (data, 155); // One extra byte detects trailing data; bound the read.
    return sendEditBufferPatch(data);
}

bool BSIIControlAudioProcessorEditor::sendEditBufferPatch (const juce::MemoryBlock& data)
{
    if(processor.isStoredPatchBusy()) { statusLabel.setText("Wait for the stored-preset operation to finish.",juce::dontSendNotification); return false; }
    if (midiOutput == nullptr)
    {
        statusLabel.setText ("Cannot send patch: no MIDI output is open.", juce::dontSendNotification);
        return false;
    }
    if (! bsii::isRecallPatch (data))
    {
        statusLabel.setText ("Not sent: requires one validated 154-byte edit-buffer patch. Bank, stored-patch and other commands are unsupported.",
                             juce::dontSendNotification);
        return false;
    }
    forgetPreset();
    midiOutput->sendMessageNow (juce::MidiMessage (data.getData(), static_cast<int> (data.getSize()), 0.0));
    statusLabel.setText ("Sent temporary patch to BSII edit buffer. Hardware receipt not confirmed.", juce::dontSendNotification);
    return true;
}

void BSIIControlAudioProcessorEditor::sendSysExFileToHardware()
{
    if (midiOutput == nullptr)
    {
        statusLabel.setText ("Cannot send patch: no MIDI output is open.", juce::dontSendNotification);
        return;
    }
    auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    fileChooser = std::make_unique<juce::FileChooser> ("Send a validated Bass Station II edit-buffer patch", start, "*.syx");
    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file != juce::File{}) sendEditBufferFile (file);
        fileChooser.reset();
    });
}


bool BSIIControlAudioProcessorEditor::setEditorParameterValue (const char* parameterId, int value, bool notifyHost)
{
    auto* parameter = processor.getParameters().getParameter (parameterId);
    if (parameter == nullptr)
        return false;

    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
    {
        const auto normalised = ranged->convertTo0to1 (static_cast<float> (value));
        if (notifyHost) ranged->setValueNotifyingHost (normalised);
        else
        {
            ranged->setValue (normalised);
            for (auto& control : parameterControls)
                if (juce::String (bsii::parameterSpecs()[static_cast<size_t> (control->specIndex())].id) == parameterId)
                    control->setFromHardware (value);
            for (auto& control : choiceControls)
            {
                const auto& spec = bsii::choiceSpecs()[static_cast<size_t> (control->choiceIndex())];
                if (juce::String (spec.id) == parameterId
                    && juce::isPositiveAndBelow (value, static_cast<int> (spec.choices.size())))
                    control->setFromWireValue (spec.choices[static_cast<size_t> (value)].wireValue);
            }
        }
        return true;
    }

    return false;
}

bool BSIIControlAudioProcessorEditor::decodeSysExIntoEditor (const juce::MemoryBlock& data,
                                                              const juce::String& sourceName, bool notifyHost)
{
    const auto decoded = bsii::decodePatchFields (data);
    if (! decoded.valid)
    {
        statusLabel.setText ("Could not decode " + sourceName + ": " + decoded.error,
                             juce::dontSendNotification);
        return false;
    }

    BSIIControlAudioProcessor::ScopedMidiSilence silence (processor);

    juce::StringArray changedFields;
    auto set = [this, &changedFields, notifyHost] (const char* id, int value)
    {
        if (value == bsii::noHardwareValue
            || (value < 0 && juce::String (id) != "bendAmount")) return;
        const auto before = currentParameterValue (id);
        if (setEditorParameterValue (id, value, notifyHost) && before != value)
            changedFields.add (juce::String (id) + " " + juce::String (before) + "->" + juce::String (value));
    };

    set ("portamento", decoded.portamento);
    set ("patchVolume", decoded.patchVolume);
    set ("subWave", decoded.subWave);
    set ("subOctaveRaw", decoded.subOctaveRaw);
    set ("bendAmount", decoded.bendAmount);

    set ("osc1Fine", decoded.osc1Fine);
    set ("osc1Wave", decoded.osc1Wave);
    set ("osc1Coarse", decoded.osc1Coarse);
    set ("osc1ManualPW", decoded.osc1ManualPW);
    set ("osc1ModEnvPW", decoded.osc1ModEnvPW);
    set ("osc1Range", decoded.osc1Range);
    set ("osc1ModEnvPitch", decoded.osc1ModEnvPitch);
    set ("osc1Lfo1Pitch", decoded.osc1Lfo1Pitch);
    set ("osc1Lfo2PW", decoded.osc1Lfo2PW);

    set ("osc2Fine", decoded.osc2Fine);
    set ("osc2Wave", decoded.osc2Wave);
    set ("osc2Coarse", decoded.osc2Coarse);
    set ("osc2ManualPW", decoded.osc2ManualPW);
    set ("osc2Range", decoded.osc2Range);
    set ("osc2ModEnvPitch", decoded.osc2ModEnvPitch);
    set ("osc2Lfo1Pitch", decoded.osc2Lfo1Pitch);
    set ("osc2ModEnvPW", decoded.osc2ModEnvPW);
    set ("osc2Lfo2PW", decoded.osc2Lfo2PW);

    set ("mixOsc1", decoded.mixOsc1);
    set ("mixOsc2", decoded.mixOsc2);
    set ("mixSub", decoded.mixSub);
    set ("mixNoise", decoded.mixNoise);
    set ("mixRing", decoded.mixRing);
    set ("mixExternal", decoded.mixExternal);

    set ("filterCutoff", decoded.filterCutoff);
    set ("filterResonance", decoded.filterResonance);
    set ("filterOverdrive", decoded.filterOverdrive);
    set ("filterType", decoded.filterType);
    set ("filterShape", decoded.filterShape);
    set ("filterSlope", decoded.filterSlope);
    set ("filterModEnv", decoded.filterModEnv);
    set ("filterLfo2", decoded.filterLfo2);
    set ("filterTracking", decoded.filterTracking);

    set ("velocityAmp", decoded.velocityAmp);
    set ("ampAttack", decoded.ampAttack);
    set ("ampTrigger", decoded.ampTrigger);
    set ("ampDecay", decoded.ampDecay);
    set ("ampSustain", decoded.ampSustain);
    set ("ampRelease", decoded.ampRelease);
    set ("ampRetrigger", decoded.ampRetrigger);
    set ("ampFixed", decoded.ampFixed);
    set ("ampRetriggerCount", decoded.ampRetriggerCount);

    set ("velocityMod", decoded.velocityMod);
    set ("modAttack", decoded.modAttack);
    set ("modDecay", decoded.modDecay);
    set ("modSustain", decoded.modSustain);
    set ("modRelease", decoded.modRelease);
    set ("modRetrigger", decoded.modRetrigger);
    set ("modTrigger", decoded.modTrigger);
    set ("modFixed", decoded.modFixed);
    set ("modRetriggerCount", decoded.modRetriggerCount);

    set ("lfo1Wave", decoded.lfo1Wave);
    set ("lfo1Delay", decoded.lfo1Delay);
    set ("lfo1Slew", decoded.lfo1Slew);
    set ("lfo1Speed", decoded.lfo1Speed);
    set ("lfo1KeySync", decoded.lfo1KeySync);
    set ("lfo2Wave", decoded.lfo2Wave);
    set ("lfo2Delay", decoded.lfo2Delay);
    set ("lfo2Slew", decoded.lfo2Slew);
    set ("lfo2Speed", decoded.lfo2Speed);
    set ("lfo2KeySync", decoded.lfo2KeySync);
    set ("lfo1Sync", decoded.lfo1Sync);
    set ("lfo2Sync", decoded.lfo2Sync);
    set ("lfo1Division", decoded.lfo1Division);
    set ("lfo2Division", decoded.lfo2Division);

    set ("arpSwing", decoded.arpSwing);
    set ("arpOn", decoded.arpOn);
    set ("arpLatch", decoded.arpLatch);
    set ("arpOctaves", decoded.arpOctaves);
    set ("arpMode", decoded.arpMode);
    set ("arpRhythm", decoded.arpRhythm);
    set ("seqRetrig", decoded.seqRetrig);
    set ("modWhLfo1Pitch", decoded.modWhLfo1Pitch);
    set ("paraphonic", decoded.paraphonic);
    set ("subFine", decoded.subFine);
    set ("subCoarse", decoded.subCoarse);
    set ("oscError", decoded.oscError);
    set ("glideDivergence", decoded.glideDivergence);
    set ("microTuneTable", decoded.microTuneTable);
    set ("modWhFilter", decoded.modWhFilter);
    set ("modWhLfo2Filter", decoded.modWhLfo2Filter);
    set ("modWhOsc2Pitch", decoded.modWhOsc2Pitch);
    set ("aftertouchFilter", decoded.aftertouchFilter);
    set ("aftertouchLfo1", decoded.aftertouchLfo1);
    set ("aftertouchLfo2Spd", decoded.aftertouchLfo2Spd);
    set ("oscFilterMod", decoded.oscFilterMod);
    set ("oscSyncRaw", decoded.oscSyncRaw);
    set ("distortion", decoded.distortion);
    set ("vcaLimit", decoded.vcaLimit);

    updateFilterModeUi();

    diagnosticLabel.setText (changedFields.isEmpty()
        ? "SysEx decoder changed no editor fields."
        : "SysEx changed: " + changedFields.joinIntoString (", "),
        juce::dontSendNotification);

    statusLabel.setText (
        "Decoded " + juce::String (decoded.decodedCount) + " patch fields ("
        + juce::String (decoded.verifiedCount) + " verified, "
        + juce::String (decoded.candidateCount) + " safe candidates)"
        + (decoded.patchName.isNotEmpty() ? " from \"" + decoded.patchName + "\"" : juce::String())
        + " (" + sourceName + "). Out-of-range/unknown fields were skipped.",
        juce::dontSendNotification);
    return true;
}

void BSIIControlAudioProcessorEditor::importSysExToEditor()
{
    auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    fileChooser = std::make_unique<juce::FileChooser> ("Import a captured Bass Station II .syx into the editor",
                                                        start, "*.syx");
    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file != juce::File{})
        {
            juce::MemoryBlock data;
            if (! file.loadFileAsData (data))
                statusLabel.setText ("Could not read " + file.getFileName() + ".", juce::dontSendNotification);
            else
            {
                forgetPreset();
                decodeSysExIntoEditor (data, file.getFileName());
            }
        }
        fileChooser.reset();
    });
}

void BSIIControlAudioProcessorEditor::setLiveRxPaused (bool paused)
{
    liveRxPaused = paused;
    processor.setLiveMidiRxEnabled (! paused);
    pauseLiveRxButton.setButtonText (paused ? "Live RX is paused" : "Live RX is running");
    rxLabel.setText (paused ? "CC RX: paused (SysEx still active)" : "CC RX: waiting...", juce::dontSendNotification);
    nrpnLabel.setText (paused ? "NRPN RX: paused (SysEx still active)" : "NRPN RX: waiting...", juce::dontSendNotification);
    statusLabel.setText (paused
        ? "Live CC/NRPN RX paused. Change the BSII preset/controls, then use Get Patch to validate SysEx only."
        : "Live CC/NRPN RX resumed.",
        juce::dontSendNotification);
}

void BSIIControlAudioProcessorEditor::updateFilterModeUi()
{
    const auto acid = currentParameterValue ("filterType") == 1;

    const auto& choices = bsii::choiceSpecs();
    for (size_t i = 0; i < choices.size() && i < choiceControls.size(); ++i)
    {
        const juce::String id (choices[i].id);
        if (id == "filterShape" || id == "filterSlope")
            choiceControls[i]->setInteractionEnabled (! acid);
    }

    if (acid)
    {
        BSIIControlAudioProcessor::ScopedMidiSilence silence (processor);
        setEditorParameterValue ("filterShape", 0, false); // Acid is LP only
        setEditorParameterValue ("filterSlope", 1, false); // Acid is fixed 24 dB/oct
    }
}

bool BSIIControlAudioProcessorEditor::saveStateToFile (const juce::File& requestedFile)
{
    auto file = requestedFile;
    if (! file.hasFileExtension ("bsiistate"))
        file = file.withFileExtension ("bsiistate");

    auto state = bsii::patchOnlyState (processor.getParameters().copyState());
    state.setProperty ("bsiiControlVersion", "0.8.1", nullptr);
    state.setProperty ("savedAt", juce::Time::getCurrentTime().toISO8601 (true), nullptr);

    if (auto xml = state.createXml())
        return xml->writeTo (file);
    return false;
}

bool BSIIControlAudioProcessorEditor::loadStateFromFile (const juce::File& file)
{
    auto xml = juce::parseXML (file);
    if (xml == nullptr)
        return false;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || ! state.hasType (processor.getParameters().state.getType()))
        return false;

    BSIIControlAudioProcessor::ScopedMidiSilence silence (processor);
    forgetPreset();
    processor.getParameters().replaceState (bsii::patchStateKeepingGlobals (state, processor.getParameters().copyState()));
    return true;
}

void BSIIControlAudioProcessorEditor::saveSnapshot()
{
    auto defaultFile = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                           .getChildFile ("Bass Station II Snapshot.bsiistate");
    fileChooser = std::make_unique<juce::FileChooser> ("Save Bass Station II editor snapshot", defaultFile,
                                                        "*.bsiistate");
    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file != juce::File{})
        {
            const auto ok = saveStateToFile (file);
            statusLabel.setText (ok ? "Saved snapshot: " + file.withFileExtension ("bsiistate").getFileName()
                                    : "Could not save snapshot.",
                                 juce::dontSendNotification);
        }
        fileChooser.reset();
    });
}

void BSIIControlAudioProcessorEditor::loadSnapshot()
{
    auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    fileChooser = std::make_unique<juce::FileChooser> ("Load Bass Station II editor snapshot", start,
                                                        "*.bsiistate");
    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file != juce::File{})
        {
            const auto ok = loadStateFromFile (file);
            if (ok)
            {
                statusLabel.setText ("Loaded snapshot: " + file.getFileName() + juce::String::fromUTF8 (". Use Send All → BSII to recall it on hardware."),
                                     juce::dontSendNotification);
            }
            else
            {
                statusLabel.setText ("Could not load snapshot or file is not a BSII Control state.",
                                     juce::dontSendNotification);
            }
        }
        fileChooser.reset();
    });
}

void BSIIControlAudioProcessorEditor::applyHardwareUpdates()
{
    BSIIControlAudioProcessor::ScopedMidiSilence silence (processor);
    for (auto& control : parameterControls)
    {
        const auto index = control->specIndex();
        const auto value = processor.consumeHardwareValue (index);
        if (value == bsii::noHardwareValue) continue;
        setEditorParameterValue (bsii::parameterSpecs()[static_cast<size_t> (index)].id, value, true);
    }

    for (auto& control : choiceControls)
    {
        const auto index = control->choiceIndex();
        const auto wireValue = processor.consumeChoiceWireValue (index);
        if (wireValue < 0) continue;
        const auto& spec = bsii::choiceSpecs()[static_cast<size_t> (index)];
        for (size_t i = 0; i < spec.choices.size(); ++i)
            if (spec.choices[i].wireValue == wireValue)
                setEditorParameterValue (spec.id, static_cast<int> (i), true);
        control->setFromWireValue (wireValue);
    }
}

void BSIIControlAudioProcessorEditor::updateRxDisplay()
{
    const auto rx = processor.getLatestReceivedMidi();
    const auto hex = [] (int value)
    {
        return juce::String::toHexString (value & 0xff).paddedLeft ('0', 2).toUpperCase();
    };

    if (rx.serial != 0 && rx.serial != lastDisplayedRxSerial)
    {
        lastDisplayedRxSerial = rx.serial;
        const auto statusByte = 0xB0 + juce::jlimit (0, 15, rx.channel - 1);
        rxLabel.setText ("CC RX: " + hex (statusByte) + " " + hex (rx.controller) + " " + hex (rx.value)
                         + " | Ch " + juce::String (rx.channel) + " CC " + juce::String (rx.controller)
                         + " = " + juce::String (rx.value), juce::dontSendNotification);
    }

    if (rx.nrpnSerial != 0 && rx.nrpnSerial != lastDisplayedNrpnSerial)
    {
        lastDisplayedNrpnSerial = rx.nrpnSerial;
        nrpnLabel.setText ("NRPN RX: " + juce::String (rx.nrpnMSB) + ":" + juce::String (rx.nrpnLSB)
                           + " = " + juce::String (rx.nrpnValue), juce::dontSendNotification);
    }
}
