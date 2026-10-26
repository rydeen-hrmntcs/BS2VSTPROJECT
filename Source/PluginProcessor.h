#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "BSIIMidi.h"
#include "BSIITuningDocument.h"
#include "BSIIPatchBank.h"
#include "BSIIStoredPatchWrite.h"
#include "BSIIHardwareSysExInput.h"

class BSIIControlAudioProcessor : public juce::AudioProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener,
                                  private juce::Timer
{
public:
    BSIIControlAudioProcessor();
    ~BSIIControlAudioProcessor() override;

    void prepareToPlay (double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState& getParameters() { return apvts; }

    bool usesDirectHardwareSysEx() const { return wrapperType != wrapperType_Standalone; }
    bool connectHardwareSysExInput (const juce::String& outputName)
    { return ! usesDirectHardwareSysEx() || directSysExInput.connect (outputName); }
    // Message-thread access for explicit patch/preset commands. The processor
    // retains the port when the editor is destroyed.
    std::unique_ptr<juce::MidiOutput>& hardwareMidiOutput() { return midiOutput; }
    int getHardwareMidiChannel() const { return hardwareMidiChannel.load(); }
    void rememberHardwareMidiOutput (const juce::String& identifier, const juce::String& name);
    bool hasSavedHardwareConnection() const;

    enum class RecallMode { off = 0, preset = 1, capturedSound = 2 };
    struct ProjectRecallInfo
    {
        RecallMode mode = RecallMode::off;
        int program = -1;
        bool hasCapture = false, capturing = false;
        juce::String status;
    };
    ProjectRecallInfo getProjectRecallInfo() const;
    bool configureProjectRecall (RecallMode, int program);
    bool captureProjectSound(); // Fresh edit-buffer request; does not send a sound.

    // Local table work only; never queued as MIDI or applied by patch recall.
    struct LocalTuningInfo
    {
        bsii::microtuning::Document document;
        bool unsavedFile = false;
        bool canUndo = false, canRedo = false;
        uint64_t revision = 0;
    };
    LocalTuningInfo getLocalTuningInfo() const;
    bool replaceLocalTuningDocument (const bsii::microtuning::Document&, bool unsavedFile = true);
    void markLocalTuningFileSaved();
    bool undoLocalTuning();
    bool redoLocalTuning();

    // Message-thread-only browser workspace; survives editor closure, not Set state.
    std::shared_ptr<const bsii::PatchBank> getLocalBank() const { return localBank; }
    int getLocalBankSelection() const { return localBankSelection; }
    void setLocalBank (std::shared_ptr<const bsii::PatchBank> bank)
    { localBank = std::move(bank); localBankSelection = localBank ? 0 : -1; }
    void selectLocalBankEntry (int index)
    { if (localBank && juce::isPositiveAndBelow(index,128)) localBankSelection = index; }

    struct StoredWriteInfo
    {
        bool busy=false, ready=false;
        int destination=-1, sourceIndex=-1, progress=0;
        uint64_t token=0;
        juce::String source, status, originalName, plannedHash;
        juce::File backupFolder;
    };
    StoredWriteInfo getStoredWriteInfo() const;
    bool prepareStoredPatchWrite(int destination, const juce::File& backupRoot = {});
    bool confirmStoredPatchWrite(uint64_t token);
    void cancelStoredPatchWrite();
    bool isStoredPatchBusy() const { return storedWrite.busy(); }

    // Reception/import/recall updates parameters without transmitting them.
    // Thread-local scopes leave concurrent automation on other threads enabled.
    class ScopedMidiSilence
    {
    public:
        explicit ScopedMidiSilence (BSIIControlAudioProcessor&, bool discardPendingTrigger = true);
        ~ScopedMidiSilence();
    private:
        BSIIControlAudioProcessor& processor;
        ScopedMidiSilence* previous;
        bool discardPendingTrigger;
        friend class BSIIControlAudioProcessor;
        JUCE_DECLARE_NON_COPYABLE (ScopedMidiSilence)
    };
    void clearPendingMidi(); // Message thread, before explicit full-state sends.
    void setHardwareMidiChannel (int channel);
    // Message-thread administrative command, never an automatable sound parameter.
    bool requestHardwareMidiChannel (int target);
    struct HardwareChannelInfo { int pending = -1, reported = -1; bool timedOut = false; };
    HardwareChannelInfo getHardwareChannelInfo() const;

    void setLiveMidiRxEnabled (bool enabled)
    {
        if (! enabled)
            for (auto& value : currentNrpnDataMSB) value.store (-1, std::memory_order_relaxed);
        liveMidiRxEnabled.store (enabled, std::memory_order_relaxed);
    }
    bool isLiveMidiRxEnabled() const { return liveMidiRxEnabled.load (std::memory_order_relaxed); }
    // Returns bsii::noHardwareValue when no update is pending (negative bend is valid).
    int getKnownHardwareProgram() const { return knownHardwareProgram.load(); }
    void rememberHardwareProgram (int program) { knownHardwareProgram.store (program); }
    int consumeHardwareProgram() { return pendingHardwareProgram.exchange (-1, std::memory_order_acq_rel); }
    int consumeHardwareValue (int specIndex);
    int consumeChoiceWireValue (int choiceIndex);

    struct ReceivedMidi
    {
        int channel = 0;
        int controller = 0;
        int value = 0;
        int nrpnMSB = -1;
        int nrpnLSB = -1;
        int nrpnValue = -1;
        uint32_t serial = 0;
        uint32_t nrpnSerial = 0;
    };

    ReceivedMidi getLatestReceivedMidi() const;

    struct ReceivedSysEx
    {
        juce::MemoryBlock data;
        uint32_t serial = 0;
    };

    ReceivedSysEx getLatestSysEx() const;

private:
    void parameterChanged (const juce::String&, float) override;
    void timerCallback() override;
    void restoreHardwareConnection();
    bool serviceProjectRecall();
    bool serviceHardwareChannelChange();
    bool midiIsSilenced() const;
    static thread_local ScopedMidiSilence* midiSilence;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void parseIncomingMidi (const juce::MidiBuffer&, bool acceptSysEx = true, bool directControls = false);
    void receiveSysEx (const juce::MidiMessage&);
    void publishNRPN (int channel, int nrpnMSB, int nrpnLSB, int value);

    bool serviceStoredPatchWrite();
    juce::String storedConnectionKey() const;
    bsii::StoredPatchWrite storedWrite;
    juce::File storedBackupFolder;
    juce::String storedWriteError;
    juce::MemoryBlock storedPlanDocument;
    std::atomic<bool> cancelStoredWriteRequested { false };

    std::shared_ptr<const bsii::PatchBank> localBank;
    int localBankSelection = -1;
    juce::AudioProcessorValueTreeState apvts;
    mutable juce::CriticalSection localTuningLock;
    LocalTuningInfo localTuning;
    std::vector<bsii::microtuning::Document> localTuningPast, localTuningFuture;
    juce::String localTuningSavedDocument = bsii::microtuning::serialise (bsii::microtuning::Document {}).value();
    bool travelLocalTuningHistory (bool undo);
    std::unique_ptr<juce::MidiOutput> midiOutput;
    mutable juce::CriticalSection connectionLock;
    juce::String savedOutputIdentifier, savedOutputName;
    bool connectionSaved = false;
    bool connectionRestorePending = false;
    uint32_t lastConnectionAttemptMs = 0, lastPortCheckMs = 0;
    RecallMode recallMode = RecallMode::off;
    int recallProgram = -1;
    juce::MemoryBlock projectSound;
    bool recallPending = false, capturePending = false, recallSettling = false;
    uint32_t captureAfterSerial = 0, captureStartedMs = 0, recallStartedMs = 0;
    juce::String recallStatus { "Automatic recall is off." };
    std::atomic<int> hardwareMidiChannel { 1 };
    std::atomic<int> pendingHardwareChannel { -1 }, observedHardwareChannel { -1 }, reportedHardwareChannel { -1 };
    std::atomic<bool> hardwareChannelTimedOut { false };
    uint32_t hardwareChannelRequestMs = 0;

    std::atomic<int> pendingHardwareProgram { -1 };
    std::atomic<int> knownHardwareProgram { -1 };
    std::atomic<bool> liveMidiRxEnabled { true };

    static constexpr size_t maxSpecs = 96;
    static constexpr size_t maxChoices = 48;
    // Callbacks only publish atomics; native port I/O runs on the message thread.
    std::array<std::atomic<int>, maxSpecs + maxChoices> pendingMidi {};
    std::atomic<int> pendingTrigger { -1 };
    std::array<std::atomic<int>, maxSpecs> pendingHardwareValues {};
    std::array<std::atomic<int>, maxChoices> pendingChoiceWireValues {};
    std::array<std::atomic<int>, 128> pairedMSB {};

    std::array<std::atomic<int>, 17> currentNrpnMSB {};
    std::array<std::atomic<int>, 17> currentNrpnLSB {};
    std::array<std::atomic<int>, 17> currentNrpnDataMSB {};

    std::atomic<int> lastRxChannel { 0 };
    std::atomic<int> lastRxController { 0 };
    std::atomic<int> lastRxValue { 0 };
    std::atomic<uint32_t> lastRxSerial { 0 };

    std::atomic<int> lastNrpnMSB { -1 };
    std::atomic<int> lastNrpnLSB { -1 };
    std::atomic<int> lastNrpnValue { -1 };
    std::atomic<uint32_t> lastNrpnSerial { 0 };

    mutable juce::CriticalSection sysExLock;
    juce::MemoryBlock lastSysExData;
    std::atomic<uint32_t> lastSysExSerial { 0 };

    // Declared last: stop native callbacks before destroying receive state.
    bsii::HardwareSysExInput directSysExInput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BSIIControlAudioProcessor)
};
