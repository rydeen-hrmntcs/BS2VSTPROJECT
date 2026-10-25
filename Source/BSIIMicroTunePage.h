#pragma once
#include <JuceHeader.h>
#include <functional>
#include <limits>
#include "PluginProcessor.h"

class MicroTunePage : public juce::Component, private juce::TableListBoxModel, private juce::Timer
{
public:
    explicit MicroTunePage (BSIIControlAudioProcessor&);
    ~MicroTunePage() override;
    void resized() override;
    bool loadDocumentFile (const juce::File&);
    bool saveDocumentFile (const juce::File&);

private:
    class CentsCell;
    class TuningView;
    int getNumRows() override { return 128; }
    void paintRowBackground (juce::Graphics&, int, int, int, bool) override;
    void paintCell (juce::Graphics&, int, int, int, int, bool) override;
    juce::Component* refreshComponentForCell (int, int, bool, juce::Component*) override;
    void timerCallback() override;
    void refresh();
    void editCents (int, const juce::String&);
    void commitName();
    void confirmReplace (std::function<void()>);
    void chooseLoad();
    void chooseSave();
    void chooseScala();
    void newTable();
    void bulkEdit (bsii::microtuning::BulkEdit);
    void selectedRowsChanged (int) override;

    BSIIControlAudioProcessor& processor;
    BSIIControlAudioProcessor::LocalTuningInfo shown;
    uint64_t displayedRevision = std::numeric_limits<uint64_t>::max();
    juce::Component content;
    juce::Viewport viewport;
    juce::Label heading, hint, nameLabel, targetLabel, status;
    juce::TextEditor nameEditor;
    juce::ComboBox target;
    juce::TextButton newButton { "New Table" }, loadButton { "Load Table..." }, saveButton { "Save Table..." }, importButton { "Import Tuning..." };
    juce::TextButton undoButton { "Undo" }, redoButton { "Redo" }, allButton { "Select All" },
        setButton { "Set Offset" }, addButton { "Add Cents" }, resetButton { "Reset Notes" };
    juce::TextEditor bulkCents;
    juce::Label selectionHint;
    juce::TableListBox table { "MicroTune notes", this };
    std::unique_ptr<TuningView> tuningView;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<juce::DialogWindow> importDialog;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicroTunePage)
};
