#include <CoreFoundation/CoreFoundation.h>
#include "PluginEditor.h"
#include "BSIIMicroTunePage.h"
#include "BSIIScalaImportPanel.h"
#include "BSIIPatchState.h"
#include <iostream>
#include <cstring>

namespace
{
void pump (int ms = 150) { CFRunLoopRunInMode (kCFRunLoopDefaultMode, ms / 1000.0, false); }
int failures = 0;
void check (bool ok, const char* label)
{
    if (! ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
struct Capture : juce::MidiInputCallback
{
    std::atomic<int> messages { 0 };
    juce::CriticalSection lock;
    juce::MemoryBlock last;
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message) override
    {
        const juce::ScopedLock guard (lock);
        last = juce::MemoryBlock (message.getRawData(), static_cast<size_t> (message.getRawDataSize()));
        ++messages;
    }
    bool received (const juce::MemoryBlock& data)
    { const juce::ScopedLock guard (lock); return last == data; }
};
struct Listener : juce::AudioProcessorListener
{
    int parameters = 0, localChanges = 0;
    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override { ++parameters; }
    void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails& change) override
    { if (change.nonParameterStateChanged) ++localChanges; }
};
juce::TableListBox* findTable (juce::Component& component)
{
    if (auto* table = dynamic_cast<juce::TableListBox*> (&component)) return table;
    for (auto* child : component.getChildren()) if (auto* table = findTable (*child)) return table;
    return nullptr;
}
MicroTunePage* showPage (juce::AudioProcessorEditor& editor)
{
    for (auto* child : editor.getChildren())
        if (auto* tabs = dynamic_cast<juce::TabbedComponent*> (child))
        {
            const auto index = tabs->getTabNames().indexOf ("MicroTune");
            if (index < 0) return nullptr;
            tabs->setCurrentTabIndex (index);
            pump();
            return dynamic_cast<MicroTunePage*> (tabs->getTabContentComponent (index));
        }
    return nullptr;
}
bool edit (juce::TableListBox& table, int key, const juce::String& text)
{
    table.selectRow (key);
    table.scrollToEnsureRowIsOnscreen (key);
    table.updateContent();
    pump();
    auto* cell = dynamic_cast<juce::Label*> (table.getCellComponent (3, key));
    if (! cell) return false;
    cell->setText (text, juce::sendNotificationSync);
    pump();
    return true;
}
template <typename T, typename Predicate>
T* findComponent (juce::Component& parent, Predicate predicate)
{
    if (auto* match = dynamic_cast<T*> (&parent); match && predicate (*match)) return match;
    for (auto* child : parent.getChildren())
        if (auto* match = findComponent<T> (*child, predicate)) return match;
    return nullptr;
}
bool click (juce::Component& page, const juce::String& text)
{
    auto* button = findComponent<juce::TextButton> (page, [&] (auto& b) { return b.getButtonText() == text; });
    if (! button || ! button->isEnabled()) return false;
    button->triggerClick(); pump();
    return true;
}

bool picture (juce::Component& component, const juce::File& path)
{
    const auto image = component.createComponentSnapshot (component.getLocalBounds());
    juce::FileOutputStream output (path);
    return output.openedOk() && juce::PNGImageFormat().writeImageToStream (image, output);
}
}

int main (int argc, char** argv)
{
    if (argc != 2) return 2;
    juce::ScopedJuceInitialiser_GUI initialise;
    const juce::File report (argv[1]);
    if (! report.createDirectory()) return 2;
    Capture capture;
    const auto name = "BSII Local Tuning Test " + juce::Uuid().toString();
    auto sink = juce::MidiInput::createNewDevice (name, &capture);
    if (! sink) return 2;
    sink->start();
    juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Standalone);
    BSIIControlAudioProcessor processor;
    juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Undefined);
    for (int n = 0; n < 20 && ! processor.hardwareMidiOutput(); ++n)
    {
        for (const auto& port : juce::MidiOutput::getAvailableDevices())
            if (port.name == name) processor.hardwareMidiOutput() = juce::MidiOutput::openDevice (port.identifier);
        if (! processor.hardwareMidiOutput()) pump (50);
    }
    if (! processor.hardwareMidiOutput()) return 2;
    Listener listener;
    processor.addListener (&listener);
    const auto parametersBefore = processor.getParameters().copyState();
    juce::MemoryBlock originalState;
    processor.getStateInformation (originalState);
    auto editor = std::unique_ptr<juce::AudioProcessorEditor> (processor.createEditor());
    editor->setVisible (true);
    auto* page = showPage (*editor);
    auto* table = page ? findTable (*page) : nullptr;
    check (page && table && table->getNumRows() == 128, "MicroTune tab and all 128 rows");
    if (! page || ! table) return 1;
    check (edit (*table,69,"+50"), "editable cents cell");
    check (processor.getLocalTuningInfo().document.table.pitch (69) == 69.5, "UI edits actual working table");
    check (click (*page,"Undo") && processor.getLocalTuningInfo().document.table.pitch (69) == 69,
           "UI undo restores single note");
    check (click (*page,"Redo") && processor.getLocalTuningInfo().document.table.pitch (69) == 69.5,
           "UI redo reapplies note");
    auto* visual = findComponent<juce::Component> (*page, [] (auto& c) { return c.getComponentID() == "tuningVisual"; });
    check (visual != nullptr, "interactive tuning visualization available");
    if (visual)
    {
        const auto revision = processor.getLocalTuningInfo().revision;
        const auto choose = [&] (float x, float y, juce::ModifierKeys mods = {})
        {
            const juce::Point<float> point (x, y);
            const auto now = juce::Time::getCurrentTime();
            visual->mouseDown (juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(),
                point, mods, 1.0f, 0, 0, 0, 0, visual, visual, now, point, now, 1, false));
        };
        choose (58, 80);
        check (table->isRowSelected (0) && table->getNumSelectedRows() == 1, "graph selects first MIDI key");
        const auto whiteWidth = static_cast<float> (visual->getWidth() - 16) / 7.0f;
        choose (8 + whiteWidth, 175);
        check (table->isRowSelected (1), "black keyboard key takes precedence over white key");
        visual->keyPressed (juce::KeyPress (juce::KeyPress::rightKey));
        check (table->isRowSelected (2), "visual arrow navigation follows selected note");
        choose (static_cast<float> (visual->getWidth() - 14), 80);
        visual->keyPressed (juce::KeyPress (juce::KeyPress::rightKey));
        check (table->isRowSelected (127), "graph and arrow navigation clamp at final MIDI key");
        choose (static_cast<float> (visual->getWidth() - 16), 217);
        check (table->isRowSelected (127), "last partial keyboard octave selects valid final white key");
        table->selectRow (69);
        visual->keyPressed (juce::KeyPress (juce::KeyPress::leftKey));
        check (table->isRowSelected (68), "table selection updates visual keyboard focus");
        table->selectRow (69);
        check (click (*visual, "Next octave >") && table->isRowSelected (81), "next octave preserves note class");
        check (click (*visual, "< Previous octave") && table->isRowSelected (69), "previous octave returns to original note");
        table->selectRow (0);
        check (! click (*visual, "< Previous octave"), "previous octave disabled at MIDI lower bound");
        table->selectRow (119);
        check (click (*visual, "Next octave >") && table->isRowSelected (127), "next octave clamps partial final octave");
        check (! click (*visual, "Next octave >"), "next octave disabled at MIDI upper bound");
        table->selectRow (69);
        check (picture (*visual, report.getChildFile ("tuning-visual.png")), "selected A4 plus 50 cents visual render");
        check (processor.getLocalTuningInfo().revision == revision,
               "visual selection never edits the tuning document");
    }
    auto* bulk = findComponent<juce::TextEditor> (*page, [] (auto& e) { return e.getComponentID() == "bulkCents"; });
    check (bulk != nullptr, "bulk cents input available");
    if (bulk)
    {
        table->deselectAllRows(); table->selectRangeOfRows (60,61); bulk->setText ("25",false);
        check (click (*page,"Set Offset") && processor.getLocalTuningInfo().document.table.pitch (60) == 60.25
               && processor.getLocalTuningInfo().document.table.pitch (61) == 61.25, "bulk set from actual selection");
        check (click (*page,"Add Cents") && processor.getLocalTuningInfo().document.table.pitch (60) == 60.5,
               "bulk add uses current pitch");
        check (click (*page,"Undo") && processor.getLocalTuningInfo().document.table.pitch (60) == 60.25
               && processor.getLocalTuningInfo().document.table.pitch (61) == 61.25, "whole bulk operation is one undo");
        check (click (*page,"Redo") && processor.getLocalTuningInfo().document.table.pitch (61) == 61.5,
               "whole bulk operation redoes");
        check (click (*page,"Reset Notes") && processor.getLocalTuningInfo().document.table.pitch (60) == 60,
               "bulk reset from UI");
        table->deselectAllRows(); table->selectRow (0); table->selectRow (60,false,false); bulk->setText ("-50",false);
        const auto before = processor.getLocalTuningInfo().revision;
        check (click (*page,"Set Offset") && processor.getLocalTuningInfo().revision == before
               && processor.getLocalTuningInfo().document.table.pitch (60) == 60, "invalid bulk edit is atomic and adds no history");
        bulk->setText ("not a number",false);
        check (click (*page,"Add Cents") && processor.getLocalTuningInfo().revision == before, "invalid bulk text rejected");
        check (click (*page,"Select All") && table->getNumSelectedRows() == 128, "all notes selectable");
        table->deselectAllRows();
        bulk->setText ("0",false);
    }
    check (edit (*table,69,"not a number") && processor.getLocalTuningInfo().document.table.pitch (69) == 69.5,
           "invalid text keeps table");
    check (edit (*table,127,"+50") && processor.getLocalTuningInfo().document.table.pitch (127) == 127,
           "boundary edit rejected");
    check (edit (*table,60,"-50") && processor.getLocalTuningInfo().document.table.pitch (60) == 59.5,
           "negative offset from UI");
    const auto savedFile = report.getChildFile ("working.json");
    check (page->saveDocumentFile (savedFile) && ! processor.getLocalTuningInfo().unsavedFile, "save local document");
    check (processor.undoLocalTuning() && processor.getLocalTuningInfo().unsavedFile, "undo after save marks changed content unsaved");
    check (processor.redoLocalTuning() && ! processor.getLocalTuningInfo().unsavedFile, "redo to saved document restores clean file status");
    check (edit (*table,69,"0") && processor.getLocalTuningInfo().document.table.pitch (69) == 69, "independent second edit");
    check (page->loadDocumentFile (savedFile) && processor.getLocalTuningInfo().document.table.pitch (69) == 69.5,
           "load local document");
    const auto badFile = report.getChildFile ("invalid.json");
    badFile.replaceWithText ("{}");
    check (! page->loadDocumentFile (badFile) && processor.getLocalTuningInfo().document.table.pitch (69) == 69.5,
           "failed load preserves current document");
    check (! page->saveDocumentFile (report.getChildFile ("missing-directory/output.json")), "failed save reported");
    check (parametersBefore.isEquivalentTo (processor.getParameters().copyState()), "no APVTS parameter changes");
    check (listener.parameters == 0 && listener.localChanges > 0, "host sees document changes, not automation gestures");
    juce::MemoryBlock state;
    processor.getStateInformation (state);
    auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    check (xml && xml->hasAttribute ("bsiiLocalTuningDocument"), "host state includes separate local document");
    check (! bsii::patchOnlyState (processor.getParameters().copyState()).hasProperty ("bsiiLocalTuningDocument"),
           "patch snapshot state has no local table");
    table->scrollToEnsureRowIsOnscreen (60); pump();
    check (picture (*editor,report.getChildFile ("microtune-default.png")), "default-size page render");
    editor->setSize (760,600); pump();
    check (picture (*editor,report.getChildFile ("microtune-small.png")), "minimum-size page render");
    for (auto* child : page->getChildren())
        if (auto* viewport = dynamic_cast<juce::Viewport*> (child)) viewport->setViewPosition (0,10000);
    pump();
    check (picture (*editor,report.getChildFile ("microtune-small-notes.png")), "minimum-size notes reachable by scrolling");
    {
        BSIIControlAudioProcessor fresh;
        fresh.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
        check (fresh.getLocalTuningInfo().document.table.pitch (69) == 69.5
               && fresh.getLocalTuningInfo().document.table.pitch (60) == 59.5,
               "document restored in a new processor instance");
    }
    editor.reset();
    editor.reset (processor.createEditor());
    editor->setVisible (true);
    page = showPage (*editor);
    table = page ? findTable (*page) : nullptr;
    check (table && processor.getLocalTuningInfo().document.table.pitch (60) == 59.5,
           "working table survives editor recreation");
    check (processor.getLocalTuningInfo().canUndo, "history survives editor recreation");
    processor.setStateInformation (originalState.getData(), static_cast<int> (originalState.getSize())); pump();
    check (processor.getLocalTuningInfo().document.table.pitch (69) == 69, "restore identity document");
    check (! processor.getLocalTuningInfo().canUndo && ! processor.getLocalTuningInfo().canRedo, "host restore starts fresh history");
    processor.setStateInformation (state.getData(), static_cast<int> (state.getSize())); pump();
    check (processor.getLocalTuningInfo().document.table.pitch (69) == 69.5
           && ! processor.getParameters().state.hasProperty ("bsiiLocalTuningDocument"), "restore strips metadata from APVTS");
    if (xml)
    {
        xml->setAttribute ("bsiiLocalTuningDocument", "{}");
        juce::MemoryBlock malformed;
        juce::AudioProcessor::copyXmlToBinary (*xml, malformed);
        processor.setStateInformation (malformed.getData(), static_cast<int> (malformed.getSize())); pump();
        check (processor.getLocalTuningInfo().document.table.pitch (69) == 69, "malformed session document safely resets");
        xml->removeAttribute ("bsiiLocalTuningDocument");
        juce::AudioProcessor::copyXmlToBinary (*xml, malformed);
        processor.setStateInformation (malformed.getData(), static_cast<int> (malformed.getSize())); pump();
        check (processor.getLocalTuningInfo().document.table.pitch (69) == 69, "old session without metadata loads");
    }
    {
        BSIIControlAudioProcessor history;
        auto document = history.getLocalTuningInfo().document;
        for (int i = 1; i <= 70; ++i)
        {
            document.table.setCents (60, i);
            history.replaceLocalTuningDocument (document);
        }
        int count = 0;
        while (history.undoLocalTuning()) ++count;
        check (count == 64, "history bounded to latest 64 changes");
        check (history.redoLocalTuning(), "redo available after undo");
        document = history.getLocalTuningInfo().document;
        history.replaceLocalTuningDocument (document);
        check (history.getLocalTuningInfo().canRedo, "no-op does not discard redo");
        document.name = "New branch";
        history.replaceLocalTuningDocument (document);
        check (! history.getLocalTuningInfo().canRedo, "new document edit discards redo branch");
        check (history.undoLocalTuning() && history.getLocalTuningInfo().document.name != "New branch",
               "metadata changes included in history");
    }
    {
        auto before = processor.getLocalTuningInfo().document;
        before.table.setCents (60,35); processor.replaceLocalTuningDocument (before);
        const auto scaleFile = report.getChildFile ("own-scale.scl");
        scaleFile.replaceWithText ("! test fixture\nTwelve equal steps\n12\n100.0\n200.0\n300.0\n400.0\n500.0\n600.0\n700.0\n800.0\n900.0\n1000.0\n1100.0\n2/1\n");
        ScalaImportPanel panel (processor);
        panel.setVisible (true);
        check (panel.loadScaleFile (scaleFile), "Scala file selection reads without applying");
        auto* hz = findComponent<juce::TextEditor> (panel, [] (auto& e) { return e.getComponentID() == "scalaHz"; });
        auto* keep = findComponent<juce::ToggleButton> (panel, [] (auto&) { return true; });
        check (hz && keep, "Scala reference and preservation controls");
        if (hz && keep)
        {
            hz->setText ("432",true); pump(); panel.rebuildPreview();
            const auto revision = processor.getLocalTuningInfo().revision;
            check (! panel.applyPreview() && processor.getLocalTuningInfo().revision == revision, "preview does not mutate and incomplete import blocks by default");
            check (picture (panel,report.getChildFile ("scala-preview.png")), "Scala preview render");
            keep->setToggleState (true,juce::dontSendNotification); panel.rebuildPreview();
            check (click (panel,"Apply to Local Table") && processor.getLocalTuningInfo().document.table.pitch (0) == before.table.pitch (0)
                   && std::abs (*processor.getLocalTuningInfo().document.table.pitch (69) - (69+12*std::log2 (432.0/440))) < 1.0e-9,
                   "explicit keep applies valid notes atomically from preview");
            check (processor.undoLocalTuning() && processor.getLocalTuningInfo().document.table.pitch (60) == before.table.pitch (60), "Scala import is one undo step");
            check (processor.redoLocalTuning(), "Scala import can redo");
            const auto encoded = bsii::microtuning::serialise (processor.getLocalTuningInfo().document);
            const auto roundTrip = encoded ? bsii::microtuning::parseDocument (*encoded) : std::nullopt;
            check (roundTrip && roundTrip->table.pitch (69) == processor.getLocalTuningInfo().document.table.pitch (69), "Scala result round trips local JSON");
            panel.rebuildPreview();
            hz->setText ("433",true);
            const auto beforePendingText = processor.getLocalTuningInfo().revision;
            check (! panel.applyPreview() && processor.getLocalTuningInfo().revision == beforePendingText,
                   "pending text notification cannot apply an older reference preview");
            pump(); panel.rebuildPreview();
            auto changed = processor.getLocalTuningInfo().document; changed.name = "Changed since preview";
            processor.replaceLocalTuningDocument (changed);
            check (! panel.applyPreview() && processor.getLocalTuningInfo().document.name == changed.name, "stale preview cannot replace newer document");
        }
        const auto mapping = report.getChildFile ("own-mapping.kbm");
        mapping.replaceWithText ("0\n0\n127\n60\n69\n440\n0\n");
        check (panel.loadMappingFile (mapping) && hz && ! hz->isEnabled(), "KBM reference replaces manual reference controls");
        panel.rebuildPreview();
        check (panel.applyPreview() && std::abs (*processor.getLocalTuningInfo().document.table.pitch (69) - 69) < 1.0e-9,
               "KBM import uses file reference frequency");
        const auto tunFile = report.getChildFile ("own-table.tun");
        juce::String tunText = "; independent numeric export fixture\n[Exact Tuning]\n";
        for (int key = 0; key < 128; ++key) tunText += "note " + juce::String (key) + "=" + juce::String (key*100.0-31.7667,4) + "\n";
        tunFile.replaceWithText (tunText);
        check (panel.loadScaleFile (tunFile) && hz && ! hz->isEnabled() && keep && ! keep->getToggleState(),
               "TUN selection disables manual reference and resets preservation policy");
        check (! panel.loadMappingFile (mapping), "KBM cannot be combined with TUN");
        const auto beforeTun = processor.getLocalTuningInfo();
        panel.rebuildPreview();
        check (! panel.applyPreview() && processor.getLocalTuningInfo().revision == beforeTun.revision,
               "TUN preview blocks unavailable pitches by default");
        check (picture (panel,report.getChildFile ("tun-preview.png")), "TUN preview render");
        if (keep)
        {
            keep->setToggleState (true,juce::dontSendNotification); panel.rebuildPreview();
            check (click (panel,"Apply to Local Table")
                   && processor.getLocalTuningInfo().document.table.pitch (0)==beforeTun.document.table.pitch (0)
                   && std::abs (*processor.getLocalTuningInfo().document.table.pitch (69) - 68.682333) < 1.0e-9,
                   "TUN apply preserves unavailable notes and exact file pitches");
            check (processor.undoLocalTuning()
                   && processor.getLocalTuningInfo().document.table.pitch (69)==beforeTun.document.table.pitch (69),
                   "TUN import undoes in one step");
            check (processor.redoLocalTuning(), "TUN import redoes");
            const auto savedTun = report.getChildFile ("imported-tun.json");
            check (page->saveDocumentFile (savedTun), "imported TUN saves as native JSON");
            const auto roundTrip = bsii::microtuning::parseDocument (savedTun.loadFileAsString());
            check (roundTrip && roundTrip->table.pitch (69)==processor.getLocalTuningInfo().document.table.pitch (69),
                   "TUN-to-JSON round trip retains pitch");
        }
        const juce::String v2Prefix = "[Scale Begin]\nFormat=\"AnaMark-TUN\"\nFormatVersion=200\n[Info]\nName=\"Formula and mapping\"\nID=\"V2_test\"\n";
        const auto v2Text = v2Prefix + "[Functional Tuning]\nInitEqual=(69,440)\nnote 69=\"%50\"\nnote 70=\"#0\"\n[Mapping]\nKeyboard 60=69\n[Scale End]\n";
        tunFile.replaceWithText (v2Text);
        check (panel.loadScaleFile (tunFile), "TUN v2 selection");
        const auto beforeV2 = processor.getLocalTuningInfo();
        panel.rebuildPreview();
        check (!panel.applyPreview() && processor.getLocalTuningInfo().revision==beforeV2.revision,
               "muted TUN v2 key requires explicit preservation");
        if (auto* previewTable=findTable(panel)) previewTable->scrollToEnsureRowIsOnscreen(70);
        pump();
        check (picture(panel,report.getChildFile("tun-v2-preview.png")), "formula/mute preview render");
        if (keep)
        {
            keep->setToggleState(true,juce::dontSendNotification);panel.rebuildPreview();
            check (click(panel,"Apply to Local Table")
                   && std::abs(*processor.getLocalTuningInfo().document.table.pitch(60)-69.5)<1e-9
                   && processor.getLocalTuningInfo().document.table.pitch(70)==beforeV2.document.table.pitch(70),
                   "v2 mapping applies formula pitch and retains muted key explicitly");
            check (processor.undoLocalTuning() && processor.getLocalTuningInfo().document.table.pitch(60)==beforeV2.document.table.pitch(60),
                   "v2 whole import undoes once");
            check (processor.redoLocalTuning(), "v2 whole import redoes");
            const auto fileV2=report.getChildFile("imported-v2.json");
            check (page->saveDocumentFile(fileV2), "v2 flattened document saves as JSON");
            const auto jsonV2=bsii::microtuning::parseDocument(fileV2.loadFileAsString());
            check (jsonV2 && std::abs(*jsonV2->table.pitch(60)-69.5)<1e-9, "v2-to-JSON retains mapped pitch");
            juce::MemoryBlock stateV2;processor.getStateInformation(stateV2);
            BSIIControlAudioProcessor restoredV2;restoredV2.setStateInformation(stateV2.getData(),static_cast<int>(stateV2.getSize()));
            check (std::abs(*restoredV2.getLocalTuningInfo().document.table.pitch(60)-69.5)<1e-9,
                   "v2 flattened document survives host state restore");
        }
        tunFile.replaceWithText (v2Prefix+"[Exact Tuning]\nnote 69=7100\n[Functional Tuning]\nnote 69=\"/0\"\n[Scale End]\n");
        check (panel.loadScaleFile(tunFile), "invalid v2 file selected for preview");
        const auto invalidV2Revision=processor.getLocalTuningInfo().revision;
        panel.rebuildPreview();
        check (!panel.applyPreview() && processor.getLocalTuningInfo().revision==invalidV2Revision,
               "invalid v2 formula rejects numeric compatibility fallback without document changes");
        tunFile.replaceWithText (tunText+"[Functional Tuning]\nnote 0=\"formula\"\n");
        check (panel.loadScaleFile (tunFile), "TUN selection still requires parsing");
        const auto beforeUnsupported = processor.getLocalTuningInfo().revision;
        panel.rebuildPreview();
        check (! panel.applyPreview() && processor.getLocalTuningInfo().revision==beforeUnsupported,
               "unsupported TUN cannot replace document with fallback data");
        const auto bad = report.getChildFile ("bad-scale.scl"); bad.replaceWithText ("bad");
        check (panel.loadScaleFile (bad), "malformed selection still requires parse");
        const auto revision = processor.getLocalTuningInfo().revision;
        panel.rebuildPreview();
        check (! panel.applyPreview() && processor.getLocalTuningInfo().revision == revision, "bad scale keeps current table");
    }
    check (processor.getParameters().getParameter ("paraphonic")->getParameterIndex() == 94
           && static_cast<juce::AudioProcessor&> (processor).getParameters().size() == 95, "existing automation layout retained");
    check (listener.parameters == 0, "Scala imports do not change automation parameters");
    check (click (*page,"Import Tuning..."), "MicroTune opens actual import dialog");
    juce::Component::SafePointer<juce::DialogWindow> importWindow;
    for (int i = 0; i < juce::Desktop::getInstance().getNumComponents(); ++i)
        if (auto* dialog = dynamic_cast<juce::DialogWindow*> (juce::Desktop::getInstance().getComponent (i));
            dialog && dialog->getName() == "Import Tuning - local preview") importWindow = dialog;
    check (importWindow != nullptr, "import dialog present");
    editor.reset(); pump();
    check (importWindow == nullptr, "closing editor destroys import session safely");
    pump (300);
    check (capture.messages.load() == 0, "edit/load/save/restore emits no MIDI");
    // Exercise the same selected-file send path as the actual file chooser,
    // after completing the tuning-is-silent checks above. Output stays virtual.
    editor.reset (processor.createEditor());
    auto* sender = dynamic_cast<BSIIControlAudioProcessorEditor*> (editor.get());
    check (sender != nullptr, "actual patch sender editor available");
    if (sender)
    {
        juce::MemoryBlock patch;
        check (juce::File::getCurrentWorkingDirectory().getChildFile ("tests/state-fixtures/baseline.syx").loadFileAsData (patch),
               "retained edit-buffer fixture loaded");
        const auto file = report.getChildFile ("send-test.syx");
        processor.rememberHardwareProgram (23);
        auto reject = [&] (const juce::MemoryBlock& data)
        {
            check (file.replaceWithData (data.getData(), data.getSize()), "rejected fixture written");
            check (! sender->sendEditBufferFile (file), "invalid file rejected by actual sender");
            check (processor.getKnownHardwareProgram() == 23, "rejection preserves known preset");
        };
        auto changed = patch;
        static_cast<juce::uint8*> (changed.getData())[7] = 1;
        reject (changed); // Stored/bank-shaped command, even for slot 0.
        juce::MemoryBlock bank;
        for (int i = 0; i < 128; ++i)
        {
            static_cast<juce::uint8*> (changed.getData())[8] = static_cast<juce::uint8> (i);
            bank.append (changed.getData(), changed.getSize());
        }
        reject (bank); // Synthetic; never use private user banks as test fixtures.
        for (int i = 6; i <= 12; ++i)
        {
            changed = patch; static_cast<juce::uint8*> (changed.getData())[i] = 1; reject (changed);
        }
        for (const auto bad : { 0x80, 0xf0, 0xf7 })
        {
            changed = patch; static_cast<juce::uint8*> (changed.getData())[20] = static_cast<juce::uint8> (bad); reject (changed);
        }
        changed = patch; static_cast<juce::uint8*> (changed.getData())[5] = 0x34; reject (changed);
        reject (juce::MemoryBlock (patch.getData(),153));
        changed = patch; const juce::uint8 extra = 0; changed.append (&extra,1); reject (changed);
        changed = patch; changed.append (patch.getData(),patch.getSize()); reject (changed);
        const juce::uint8 save[] {0xf0,0,0x20,0x29,0,0x33,0,0x48,0xf7};
        reject (juce::MemoryBlock (save,sizeof(save)));
        reject (juce::MemoryBlock());
        check (! sender->sendEditBufferFile (report.getChildFile ("absent-file.syx")), "missing file rejected");
        pump (300);
        check (capture.messages.load() == 0, "all rejected files emit zero MIDI");
        check (processor.getKnownHardwareProgram() == 23, "all rejection paths preserve preset identity");
        check (file.replaceWithData (patch.getData(),patch.getSize()) && sender->sendEditBufferFile (file),
               "validated single edit-buffer file accepted");
        pump (300);
        check (capture.messages.load() == 1 && capture.received (patch), "valid send emits exactly the original patch bytes once");
        check (processor.getKnownHardwareProgram() == -1, "valid temporary patch clears preset identity");
        processor.rememberHardwareProgram (23);
        processor.hardwareMidiOutput().reset();
        check (! sender->sendEditBufferFile (file) && processor.getKnownHardwareProgram() == 23,
               "missing output rejects without clearing preset identity");
    }
    // Bank browsing and selection are silent; only the explicit load sends.
    for (const auto& port : juce::MidiOutput::getAvailableDevices())
        if (port.name==name) processor.hardwareMidiOutput()=juce::MidiOutput::openDevice(port.identifier);
    BankPage* banks=nullptr;
    for (auto* child : editor->getChildren())
        if (auto* tabs=dynamic_cast<juce::TabbedComponent*>(child))
        {
            const auto index=tabs->getTabNames().indexOf("Banks");
            if (index>=0) { tabs->setCurrentTabIndex(index); banks=dynamic_cast<BankPage*>(tabs->getTabContentComponent(index)); }
        }
    check(banks!=nullptr,"Banks page available");
    if (banks)
    {
        auto* rows=findTable(*banks);
        check(rows && rows->getNumRows()==0,"bank page starts empty");
        juce::MemoryBlock fixture,bank;
        check(juce::File::getCurrentWorkingDirectory().getChildFile("tests/state-fixtures/baseline.syx").loadFileAsData(fixture),"bank fixture loaded");
        for (int i=127;i>=0;--i)
        {
            auto entry=fixture;auto* bytes=static_cast<juce::uint8*>(entry.getData());
            bytes[7]=1;bytes[8]=static_cast<juce::uint8>(i);bytes[20]=static_cast<juce::uint8>(i);
            std::fill(bytes+137,bytes+153,' ');
            const auto label="Entry "+juce::String(i);
            std::memcpy(bytes+137,label.toRawUTF8(),static_cast<size_t>(label.getNumBytesAsUTF8()));
            bank.append(entry.getData(),entry.getSize());
        }
        const auto file=report.getChildFile("synthetic-bank.syx");
        check(file.replaceWithData(bank.getData(),bank.getSize()),"synthetic bank saved");
        const auto before=processor.getParameters().copyState();
        const int count=capture.messages.load();processor.rememberHardwareProgram(23);
        check(banks->loadBankFile(file),"actual page opens complete bank");
        check(rows && rows->getNumRows()==128,"actual browser has all entries");
        if(rows) rows->selectRow(11);
        pump(300);
        check(processor.getLocalBankSelection()==11 && processor.getLocalBank()->name(11)=="Entry 11","selection indexed by entry rather than file order");
        check(capture.messages.load()==count && processor.getKnownHardwareProgram()==23
              && before.isEquivalentTo(processor.getParameters().copyState()),"opening and selecting preserve MIDI, parameters and preset");
        const auto saved=processor.getLocalBank();
        const auto bad=report.getChildFile("bad-bank.syx");bad.replaceWithData(fixture.getData(),fixture.getSize());
        check(!banks->loadBankFile(bad) && processor.getLocalBank()==saved && processor.getLocalBankSelection()==11,"invalid bank keeps previous bank and selection");
        check(picture(*banks,report.getChildFile("bank-browser.png")),"bank browser rendered");
        processor.hardwareMidiOutput().reset();
        check(!banks->loadSelectedTemporary() && capture.messages.load()==count && processor.getKnownHardwareProgram()==23,"missing output prevents bank send");
        for (const auto& port : juce::MidiOutput::getAvailableDevices())
            if(port.name==name) processor.hardwareMidiOutput()=juce::MidiOutput::openDevice(port.identifier);
        pump(300);
        const auto expected=saved->temporaryPatch(11);
        check(click(*banks,"Load Selected to BSII"),"actual explicit bank load button works");
        pump(300);
        check(capture.messages.load()==count+1 && capture.received(expected),"one converted patch sent exactly once with no CC echo");
        check(processor.getKnownHardwareProgram()==-1 && listener.parameters==0,"temporary bank load clears preset without host automation gestures");
        juce::MemoryBlock state;processor.getStateInformation(state);
        BSIIControlAudioProcessor fresh;fresh.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        check(!fresh.getLocalBank(),"bank workspace is not silently embedded/replayed by DAW state");
        editor.reset();pump();editor.reset(processor.createEditor());pump();
        check(processor.getLocalBank()==saved && processor.getLocalBankSelection()==11 && capture.messages.load()==count+1,"editor reopen keeps workspace without resending");
    }
    processor.removeListener (&listener);
    editor.reset(); sink->stop();
    std::cout << (failures == 0 ? "PASS" : "FAIL") << " MicroTune editor, local files, session restore, MIDI isolation edit-buffer file gate and bank browser ("
              << failures << " failures)\n";
    return failures == 0 ? 0 : 1;
}
