#pragma once
#include "PluginProcessor.h"
#include <functional>

class BankPage : public juce::Component, private juce::TableListBoxModel, private juce::Timer
{
public:
    BankPage (BSIIControlAudioProcessor&, std::function<bool(const juce::MemoryBlock&)>, std::function<bool()> = {});
    ~BankPage() override;
    void resized() override;
    bool loadBankFile (const juce::File&);
    bool loadSelectedTemporary();
    bool prepareSelectedWrite(int destination, const juce::File& backupRoot = {});
    bool confirmPreparedWrite(uint64_t token);
private:
    void chooseBank();
    void confirmWriteDialog();
    void refresh();
    void timerCallback() override { refresh(); }
    int getNumRows() override { return shown ? 128 : 0; }
    void paintRowBackground(juce::Graphics&,int,int,int,bool) override;
    void paintCell(juce::Graphics&,int,int,int,int,bool) override;
    void selectedRowsChanged(int) override;
    BSIIControlAudioProcessor& processor;
    std::function<bool(const juce::MemoryBlock&)> send;
    std::function<bool()> canPrepare;
    std::shared_ptr<const bsii::PatchBank> shown;
    juce::Viewport viewport;
    juce::Component content;
    juce::Label heading, hint, source, status, destinationLabel, preview;
    juce::ComboBox destination;
    juce::TextButton open { "Open Bank..." }, load { "Load Selected to BSII" };
    juce::TextButton prepare { "Prepare stored write" }, write { "Write stored preset..." }, showBackup { "Show backup" };
    juce::TableListBox table { "Bank entries", this };
    std::unique_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankPage)
};
