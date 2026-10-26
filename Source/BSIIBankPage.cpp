#include "BSIIBankPage.h"

BankPage::BankPage(BSIIControlAudioProcessor& p, std::function<bool(const juce::MemoryBlock&)> sender, std::function<bool()> ready)
    : processor(p), send(std::move(sender)), canPrepare(std::move(ready))
{
    viewport.setViewedComponent(&content,false);
    viewport.setScrollBarsShown(true,false);
    addAndMakeVisible(viewport);
    for (auto* c : std::initializer_list<juce::Component*>{&heading,&hint,&source,&status,&open,&load,&table,&destinationLabel,&destination,&prepare,&write,&showBackup,&preview})
        content.addAndMakeVisible(c);
    heading.setText("Patch Banks",juce::dontSendNotification);
    heading.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    hint.setText("Opening and selecting send nothing. Load replaces the temporary sound; capture unsaved edits first. Stop playback before loading.",juce::dontSendNotification);
    hint.setColour(juce::Label::textColourId,juce::Colours::lightblue);
    hint.setMinimumHorizontalScale(1.0f);
    source.setTooltip("This bank stays open for this plugin instance but is not embedded in the DAW Set. Keep the original .syx file.");
    status.setText("Stored writes require a fresh backup and explicit confirmation.",juce::dontSendNotification);
    destinationLabel.setText("Stored destination",juce::dontSendNotification);
    for(int i=0;i<128;++i) destination.addItem(juce::String(i),i+1);
    const auto existingDestination=processor.getStoredWriteInfo().destination;
    destination.setSelectedId(existingDestination>=0?existingDestination+1:2,juce::dontSendNotification);
    destination.onChange=[this]{ processor.cancelStoredPatchWrite(); refresh(); };
    prepare.onClick=[this]{ prepareSelectedWrite(destination.getSelectedId()-1); };
    write.onClick=[this]{ confirmWriteDialog(); };
    showBackup.onClick=[this]{ const auto folder=processor.getStoredWriteInfo().backupFolder; if(folder.isDirectory()) folder.revealToUser(); };
    preview.setMinimumHorizontalScale(1.0f);
    status.setMinimumHorizontalScale(1.0f);
    table.getHeader().addColumn("Entry",1,70,55,100);
    table.getHeader().addColumn("Patch name",2,450,120,1000);
    table.getHeader().setStretchToFitActive(true);
    table.setMultipleSelectionEnabled(false);
    table.setRowHeight(28);
    table.setColour(juce::ListBox::backgroundColourId,juce::Colour(0xff202326));
    open.onClick = [this]{ chooseBank(); };
    load.onClick = [this]{ loadSelectedTemporary(); };
    refresh();
    startTimerHz(5);
}
BankPage::~BankPage()
{
    stopTimer(); chooser.reset(); table.setModel(nullptr); viewport.setViewedComponent(nullptr,false);
}
void BankPage::resized()
{
    viewport.setBounds(getLocalBounds());
    const int height=juce::jmax(510,getHeight());
    content.setSize(juce::jmax(1,getWidth()-(height>getHeight()?viewport.getScrollBarThickness():0)),height);
    auto area=content.getLocalBounds().reduced(12);
    heading.setBounds(area.removeFromTop(26));
    hint.setBounds(area.removeFromTop(46));
    auto buttons=area.removeFromTop(34); open.setBounds(buttons.removeFromLeft(145).reduced(0,2));
    buttons.removeFromLeft(10); load.setBounds(buttons.removeFromLeft(210).reduced(0,2));
    source.setBounds(area.removeFromTop(28));
    auto stored=area.removeFromTop(32);
    destinationLabel.setBounds(stored.removeFromLeft(125));
    destination.setBounds(stored.removeFromLeft(65).reduced(0,2)); stored.removeFromLeft(8);
    prepare.setBounds(stored.removeFromLeft(165).reduced(0,2)); stored.removeFromLeft(8);
    write.setBounds(stored.removeFromLeft(170).reduced(0,2)); stored.removeFromLeft(8);
    showBackup.setBounds(stored.removeFromLeft(105).reduced(0,2));
    preview.setBounds(area.removeFromTop(62));
    auto footer=area.removeFromBottom(46); table.setBounds(area); status.setBounds(footer);
}
void BankPage::refresh()
{
    auto bank=processor.getLocalBank();
    if (bank!=shown)
    {
        shown=std::move(bank); table.updateContent();
        source.setText(shown ? shown->source()+" | 128 patches" : "No bank open",juce::dontSendNotification);
    }
    const int selected=processor.getLocalBankSelection();
    if (table.getSelectedRow()!=selected) table.selectRow(selected);
    const auto info=processor.getStoredWriteInfo();
    const bool connected=processor.hardwareMidiOutput()!=nullptr;
    open.setEnabled(!info.busy); table.setEnabled(!info.busy); destination.setEnabled(!info.busy);
    load.setEnabled(shown && juce::isPositiveAndBelow(selected,128) && connected && !info.busy);
    prepare.setEnabled(shown && connected && !info.busy && (!canPrepare || canPrepare()));
    write.setEnabled(info.ready && selected==info.sourceIndex && destination.getSelectedId()-1==info.destination && (!canPrepare || canPrepare()));
    showBackup.setEnabled(info.backupFolder.isDirectory());
    if(info.destination>=0)
    {
        preview.setText("Source: "+info.source+" / entry "+juce::String(info.sourceIndex)
            +"  ->  Stored preset "+juce::String(info.destination)+"\nOriginal: "+info.originalName
            +" | "+info.status+(info.busy?" ("+juce::String(info.progress)+"/128)":""),juce::dontSendNotification);
        preview.setTooltip(info.backupFolder.getFullPathName());
    }
    else preview.setText("Prepare reads the current sound and all 128 stored presets. Stop playback; leave hardware and other MIDI tools idle.",juce::dontSendNotification);
}
bool BankPage::loadBankFile(const juce::File& file)
{
    if(processor.isStoredPatchBusy()) return false;
    juce::String error;
    auto bank=bsii::PatchBank::readFile(file,error);
    if (!bank) { status.setText(error,juce::dontSendNotification); return false; }
    processor.cancelStoredPatchWrite();
    processor.setLocalBank(std::move(bank));
    refresh();
    status.setText("Bank opened locally. Choose an entry, then Load Selected to BSII. Stored presets are not overwritten.",juce::dontSendNotification);
    return true;
}
void BankPage::chooseBank()
{
    chooser=std::make_unique<juce::FileChooser>("Open a complete Bass Station II bank",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.syx");
    juce::Component::SafePointer<BankPage> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [safe](const juce::FileChooser& chosen)
        {
            const auto file=chosen.getResult();
            if (safe && file!=juce::File{}) safe->loadBankFile(file);
            if (safe) safe->chooser.reset();
        });
}
bool BankPage::loadSelectedTemporary()
{
    const auto bank=processor.getLocalBank();
    const auto index=processor.getLocalBankSelection();
    if (processor.isStoredPatchBusy() || !bank || !juce::isPositiveAndBelow(index,128) || !processor.hardwareMidiOutput())
    { status.setText("Not sent. Open a bank, select an entry and connect a MIDI output.",juce::dontSendNotification); return false; }
    auto patch=bank->temporaryPatch(index);
    if (!bsii::isRecallPatch(patch) || !send(patch))
    { status.setText("Not sent. Check the MIDI output and selected patch.",juce::dontSendNotification); return false; }
    status.setText("Sent entry "+juce::String(index)+" to the temporary buffer. Stored presets unchanged; receipt not confirmed.",juce::dontSendNotification);
    return true;
}
bool BankPage::prepareSelectedWrite(int target, const juce::File& backupRoot)
{
    if(canPrepare && !canPrepare()) { status.setText("Wait for the current patch refresh or capture to finish.",juce::dontSendNotification); return false; }
    destination.setSelectedId(target+1,juce::dontSendNotification);
    const bool started=processor.prepareStoredPatchWrite(target,backupRoot);
    if(!started) status.setText(processor.getStoredWriteInfo().status,juce::dontSendNotification);
    else status.setText("Read-only backup in progress. Writing may change the current sound; its backup is included.",juce::dontSendNotification);
    refresh(); return started;
}
bool BankPage::confirmPreparedWrite(uint64_t token)
{
    if(canPrepare && !canPrepare()) return false;
    const auto info=processor.getStoredWriteInfo();
    if(info.sourceIndex!=processor.getLocalBankSelection() || info.destination!=destination.getSelectedId()-1) return false;
    const bool started=processor.confirmStoredPatchWrite(token);
    refresh(); return started;
}
void BankPage::confirmWriteDialog()
{
    const auto info=processor.getStoredWriteInfo();
    if(!info.ready) return;
    juce::Component::SafePointer<BankPage> safe(this);
    juce::AlertWindow::showAsync(juce::MessageBoxOptions()
        .withIconType(juce::MessageBoxIconType::WarningIcon)
        .withTitle("Replace stored preset "+juce::String(info.destination)+"?")
        .withMessage("Copy entry "+juce::String(info.sourceIndex)+" from "+info.source+" into stored preset "
            +juce::String(info.destination)+" ("+info.originalName+").\n\nBackup: "+info.backupFolder.getFullPathName()
            +"\nWrite fingerprint: "+info.plannedHash
            +"\n\nThis overwrites that stored preset. The full bank will be read back to verify the result. Keep playback stopped and other MIDI tools idle.")
        .withButton("Write preset "+juce::String(info.destination)).withButton("Cancel"),
        [safe,token=info.token](int result){ if(safe && result==1) safe->confirmPreparedWrite(token); });
}
void BankPage::selectedRowsChanged(int row)
{
    if(row!=processor.getLocalBankSelection()) processor.cancelStoredPatchWrite();
    processor.selectLocalBankEntry(row);
    load.setEnabled(shown && juce::isPositiveAndBelow(row,128) && processor.hardwareMidiOutput()!=nullptr);
}
void BankPage::paintRowBackground(juce::Graphics& g,int row,int,int,bool selected)
{ g.fillAll(selected ? juce::Colour(0xff177b99) : (row%2?juce::Colour(0xff272b2e):juce::Colour(0xff202326))); }
void BankPage::paintCell(juce::Graphics& g,int row,int column,int width,int height,bool)
{
    if (!shown) return;
    g.setColour(juce::Colours::white);
    const auto name=shown->name(row);
    g.drawText(column==1?juce::String(row):(name.isNotEmpty()?name:"(unnamed)"),6,0,width-12,height,juce::Justification::centredLeft,true);
}
