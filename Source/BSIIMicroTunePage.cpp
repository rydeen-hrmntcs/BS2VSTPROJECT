#include "BSIIMicroTunePage.h"
#include <cstdlib>
#include "BSIIScalaImportPanel.h"

namespace
{
juce::String centsText (double pitch, int key)
{
    const auto cents = (pitch - key) * 100.0;
    return (cents > 0 ? "+" : "") + juce::String (cents, 4);
}
}

class MicroTunePage::CentsCell : public juce::Label
{
public:
    explicit CentsCell (MicroTunePage& p) : page (p)
    {
        setEditable (false, true, false);
        setJustificationType (juce::Justification::centredLeft);
        setTooltip ("Double-click to enter cents above or below this key's normal pitch. Nothing is sent to the synth.");
        onTextChange = [this] { page.editCents (row, getText()); };
    }
    void display (int key, double pitch)
    {
        row = key;
        if (! isBeingEdited()) setText (centsText (pitch, key), juce::dontSendNotification);
    }
private:
    MicroTunePage& page;
    int row = 0;
};

// A local document view: selection changes use the existing table, never MIDI.
class MicroTunePage::TuningView : public juce::Component
{
public:
    explicit TuningView (MicroTunePage& p) : page (p)
    {
        setComponentID ("tuningVisual");
        setTitle ("Tuning offsets and keyboard");
        setDescription ("Click the graph or keyboard to select a note. Arrow keys move between notes.");
        setWantsKeyboardFocus (true);
    }

    void selectionChanged()
    {
        const auto selected = page.table.getSelectedRows();
        if (! selected.contains (focusKey) && ! selected.isEmpty())
            focusKey = selected[0];
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff191d20));
        const auto pitch = page.shown.document.table.pitch (focusKey).value_or (focusKey);
        const auto note = juce::MidiMessage::getMidiNoteName (focusKey, true, true, 4);
        g.setColour (juce::Colours::white);
        g.setFont (14.0f);
        g.drawText (note + "  |  Key " + juce::String (focusKey) + "  |  "
                    + centsText (pitch, focusKey) + " cents  |  "
                    + juce::String (440.0 * std::pow (2.0, (pitch - 69.0) / 12.0), 3) + " Hz",
                    8, 0, getWidth() - 16, 24, juce::Justification::centredLeft);
        const auto plot = graphBounds();
        double limit = 50.0;
        for (int key = 0; key < 128; ++key)
            limit = juce::jmax (limit, std::abs (offset (key)));
        const auto y = [&] (double cents) { return plot.getCentreY() - static_cast<float> (cents / limit) * plot.getHeight() * 0.46f; };
        g.setFont (11.0f);
        for (const auto value : { limit, 0.0, -limit })
        {
            g.setColour (juce::Colour (0xff41494e));
            g.drawHorizontalLine (juce::roundToInt (y (value)), plot.getX(), plot.getRight());
            g.setColour (juce::Colours::lightgrey);
            g.drawText (juce::String (value, 0), 0, juce::roundToInt (y (value)) - 7, 52, 14,
                        juce::Justification::centredRight);
        }
        juce::Path line;
        for (int key = 0; key < 128; ++key)
        {
            const auto x = plot.getX() + plot.getWidth() * static_cast<float> (key) / 127.0f;
            const auto cy = y (offset (key));
            if (key == 0) line.startNewSubPath (x, cy); else line.lineTo (x, cy);
        }
        g.setColour (juce::Colour (0xff46a6c5));
        g.strokePath (line, juce::PathStrokeType (1.5f));
        for (int key = 0; key < 128; ++key)
        {
            const auto x = plot.getX() + plot.getWidth() * static_cast<float> (key) / 127.0f;
            const auto selected = page.table.isRowSelected (key);
            const auto diameter = selected ? 6.0f : 3.0f;
            g.setColour (selected ? juce::Colours::orange : juce::Colour (0xff79c9de));
            g.fillEllipse (x - diameter / 2, y (offset (key)) - diameter / 2, diameter, diameter);
        }
        g.setColour (juce::Colours::lightgrey);
        g.drawText ("Cents from equal temperament  |  All 128 keys - click to select an octave",
                    56, 145, getWidth() - 64, 18, juce::Justification::centredLeft);
        const auto start = (focusKey / 12) * 12;
        for (const auto black : { false, true })
            for (int key = start; key < juce::jmin (128, start + 12); ++key)
            {
                if (isBlack (key) != black) continue;
                const auto bounds = keyBounds (key);
                const auto selected = page.table.isRowSelected (key);
                g.setColour (selected ? juce::Colour (0xff277a96)
                                     : black ? juce::Colour (0xff24282c) : juce::Colour (0xffe0e5e7));
                g.fillRect (bounds);
                g.setColour (juce::Colour (0xff101417));
                g.drawRect (bounds);
                g.setColour (selected || black ? juce::Colours::white : juce::Colours::black);
                g.drawText (juce::MidiMessage::getMidiNoteName (key, true, true, 4), bounds.toNearestInt().withTrimmedTop (black ? 0 : 34),
                            juce::Justification::centred);
            }
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        int key = -1;
        const auto plot = graphBounds();
        if (event.position.y >= 24 && event.position.y < 164)
            key = juce::jlimit (0, 127, juce::roundToInt ((event.position.x - plot.getX()) / plot.getWidth() * 127.0f));
        else
        {
            const auto start = (focusKey / 12) * 12;
            for (const auto black : { true, false })
            {
                for (int k = start; k < juce::jmin (128, start + 12); ++k)
                    if (isBlack (k) == black && keyBounds (k).contains (event.position)) { key = k; break; }
                if (key >= 0) break;
            }
        }
        if (key >= 0) select (key, event.mods);
        grabKeyboardFocus();
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key.getKeyCode() != juce::KeyPress::leftKey && key.getKeyCode() != juce::KeyPress::rightKey) return false;
        select (juce::jlimit (0, 127, focusKey + (key.getKeyCode() == juce::KeyPress::leftKey ? -1 : 1)), key.getModifiers());
        return true;
    }

private:
    double offset (int key) const { return (page.shown.document.table.pitch (key).value_or (key) - key) * 100.0; }
    static bool isBlack (int key) { const auto pc = key % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
    juce::Rectangle<float> graphBounds() const { return { 58.0f, 32.0f, static_cast<float> (juce::jmax (1, getWidth() - 72)), 104.0f }; }
    juce::Rectangle<float> keyBounds (int key) const
    {
        constexpr int whitesBefore[] { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 };
        const auto start = (focusKey / 12) * 12;
        const auto count = start == 120 ? 5 : 7;
        const auto width = static_cast<float> (juce::jmax (1, getWidth() - 16)) / count;
        const auto x = 8.0f + whitesBefore[key % 12] * width;
        return isBlack (key) ? juce::Rectangle<float> (x - width * 0.3f, 168.0f, width * 0.6f, 35.0f)
                            : juce::Rectangle<float> (x, 168.0f, width, 58.0f);
    }
    void select (int key, juce::ModifierKeys mods)
    {
        const auto anchor = focusKey;
        focusKey = key;
        if (mods.isShiftDown()) page.table.selectRangeOfRows (juce::jmin (anchor, key), juce::jmax (anchor, key));
        else page.table.selectRow (key, false, ! mods.isCommandDown() && ! mods.isCtrlDown());
        page.table.scrollToEnsureRowIsOnscreen (key);
        repaint();
    }
    MicroTunePage& page;
    int focusKey = 60;
};

MicroTunePage::MicroTunePage (BSIIControlAudioProcessor& p) : processor (p)
{
    tuningView = std::make_unique<TuningView> (*this);
    content.addAndMakeVisible (*tuningView);
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);
    for (auto* component : std::initializer_list<juce::Component*> { &heading, &hint, &nameLabel,
            &nameEditor, &targetLabel, &target, &status, &newButton, &loadButton, &saveButton, &table, &undoButton, &redoButton, &allButton,
            &setButton, &addButton, &resetButton, &bulkCents, &selectionHint, &importButton })
        content.addAndMakeVisible (component);
    heading.setText ("MicroTune - local table", juce::dontSendNotification);
    heading.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
    hint.setText ("Edit locally. Nothing is sent to the synth. Frequencies are calculated, not measured.", juce::dontSendNotification);
    hint.setColour (juce::Label::textColourId, juce::Colours::lightblue);
    nameLabel.setText ("Name", juce::dontSendNotification);
    nameEditor.setInputRestrictions (128);
    nameEditor.onReturnKey = [this] { commitName(); };
    nameEditor.onFocusLost = [this] { commitName(); };
    targetLabel.setText ("File target", juce::dontSendNotification);
    for (int id = 0; id <= 8; ++id) target.addItem (juce::String (id), id + 1);
    target.setTooltip ("Destination stored in this local file. Does not select or change the synth's active table.");
    target.onChange = [this]
    {
        auto document = processor.getLocalTuningInfo().document;
        document.targetTable = target.getSelectedId() - 1;
        processor.replaceLocalTuningDocument (document);
        refresh();
    };
    newButton.onClick = [this] { confirmReplace ([this] { newTable(); }); };
    loadButton.onClick = [this] { confirmReplace ([this] { chooseLoad(); }); };
    importButton.onClick = [this] { commitName(); chooseScala(); };
    saveButton.onClick = [this] { chooseSave(); };
    undoButton.onClick = [this] { commitName(); processor.undoLocalTuning(); refresh(); };
    redoButton.onClick = [this] { processor.redoLocalTuning(); refresh(); };
    allButton.onClick = [this] { table.selectRangeOfRows (0, 127); };
    setButton.onClick = [this] { bulkEdit (bsii::microtuning::BulkEdit::setOffset); };
    addButton.onClick = [this] { bulkEdit (bsii::microtuning::BulkEdit::addCents); };
    resetButton.onClick = [this] { bulkEdit (bsii::microtuning::BulkEdit::reset); };
    bulkCents.setText ("0", false);
    bulkCents.setComponentID ("bulkCents");
    bulkCents.setTooltip ("Signed cents. Set Offset replaces offsets; Add Cents shifts current pitches.");
    table.setMultipleSelectionEnabled (true);
    selectedRowsChanged (-1);
    table.setRowHeight (28);
    table.getHeader().addColumn ("Key", 1, 60, 50, 100);
    table.getHeader().addColumn ("Note", 2, 85, 65, 130);
    table.getHeader().addColumn ("Offset (cents)", 3, 200, 130, 450);
    table.getHeader().addColumn ("Target (Hz)", 4, 180, 110, 400);
    table.getHeader().setStretchToFitActive (true);
    table.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff202326));
    refresh();
    startTimerHz (10);
}

MicroTunePage::~MicroTunePage()
{
    stopTimer();
    chooser.reset();
    delete importDialog.getComponent();
    table.setModel (nullptr);
    viewport.setViewedComponent (nullptr, false);
}

void MicroTunePage::resized()
{
    viewport.setBounds (getLocalBounds());
    const auto height = juce::jmax (700, getHeight());
    const auto width = juce::jmax (1, getWidth() - (height > getHeight() ? viewport.getScrollBarThickness() : 0));
    content.setSize (width, height);
    auto area = content.getLocalBounds().reduced (12);
    heading.setBounds (area.removeFromTop (26));
    hint.setBounds (area.removeFromTop (28));
    auto actions = area.removeFromTop (30);
    newButton.setBounds (actions.removeFromLeft (100)); actions.removeFromLeft (8);
    loadButton.setBounds (actions.removeFromLeft (118)); actions.removeFromLeft (8);
    saveButton.setBounds (actions.removeFromLeft (118)); actions.removeFromLeft (16);
    targetLabel.setBounds (actions.removeFromLeft (86)); target.setBounds (actions.removeFromLeft (70));
    actions.removeFromLeft (8); importButton.setBounds (actions.removeFromLeft (128));
    area.removeFromTop (8);
    auto names = area.removeFromTop (28);
    nameLabel.setBounds (names.removeFromLeft (50)); nameEditor.setBounds (names);
    area.removeFromTop (8);
    auto history = area.removeFromTop (28);
    undoButton.setBounds (history.removeFromLeft (70)); history.removeFromLeft (8);
    redoButton.setBounds (history.removeFromLeft (70)); history.removeFromLeft (12);
    selectionHint.setBounds (history);
    area.removeFromTop (6);
    auto bulk = area.removeFromTop (28);
    allButton.setBounds (bulk.removeFromLeft (90)); bulk.removeFromLeft (8);
    bulkCents.setBounds (bulk.removeFromLeft (85)); bulk.removeFromLeft (8);
    setButton.setBounds (bulk.removeFromLeft (100)); bulk.removeFromLeft (8);
    addButton.setBounds (bulk.removeFromLeft (100)); bulk.removeFromLeft (8);
    resetButton.setBounds (bulk.removeFromLeft (108));
    area.removeFromTop (8);
    tuningView->setBounds (area.removeFromTop (234));
    area.removeFromTop (8);
    status.setBounds (area.removeFromBottom (30));
    table.setBounds (area);
}

void MicroTunePage::timerCallback() { refresh(); }

void MicroTunePage::refresh()
{
    const auto latest = processor.getLocalTuningInfo();
    if (latest.revision == displayedRevision) return;
    shown = latest;
    undoButton.setEnabled (shown.canUndo);
    redoButton.setEnabled (shown.canRedo);
    displayedRevision = shown.revision;
    if (! nameEditor.hasKeyboardFocus (true)) nameEditor.setText (shown.document.name, false);
    target.setSelectedId (shown.document.targetTable + 1, juce::dontSendNotification);
    status.setText (shown.unsavedFile ? "Local edits - save a table file to keep a separate copy."
                                    : "Local working table. Save the DAW Set to keep it in this project.", juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
    table.updateContent();
    table.repaint();
    tuningView->repaint();
}

void MicroTunePage::paintRowBackground (juce::Graphics& g, int row, int width, int height, bool selected)
{
    g.fillAll (selected ? juce::Colour (0xff24556a) : row % 2 == 0 ? juce::Colour (0xff202326) : juce::Colour (0xff272b2f));
    if (std::abs (shown.document.table.pitch (row).value_or (row) - static_cast<double> (row)) > 1.0e-9)
    { g.setColour (juce::Colours::deepskyblue); g.fillRect (0, 0, 3, height); }
    juce::ignoreUnused (width);
}

void MicroTunePage::paintCell (juce::Graphics& g, int row, int column, int width, int height, bool)
{
    if (row < 0 || row >= 128 || column == 3) return;
    const auto pitch = shown.document.table.pitch (row).value_or (row);
    const auto text = column == 1 ? juce::String (row)
        : column == 2 ? juce::MidiMessage::getMidiNoteName (row, true, true, 4)
        : juce::String (440.0 * std::pow (2.0, (pitch - 69.0) / 12.0), 3);
    g.setColour (juce::Colours::white);
    g.drawText (text, 8, 0, width - 12, height, juce::Justification::centredLeft);
}

juce::Component* MicroTunePage::refreshComponentForCell (int row, int column, bool, juce::Component* existing)
{
    if (column != 3 || row < 0 || row >= 128) { delete existing; return nullptr; }
    auto* cell = dynamic_cast<CentsCell*> (existing);
    if (! cell) { delete existing; cell = new CentsCell (*this); }
    cell->display (row, shown.document.table.pitch (row).value_or (row));
    return cell;
}

void MicroTunePage::editCents (int key, const juce::String& input)
{
    const auto text = input.trim();
    char* end = nullptr;
    const auto* start = text.toRawUTF8();
    const auto cents = std::strtod (start, &end);
    auto document = processor.getLocalTuningInfo().document;
    if (text.isEmpty() || end == start || *end != 0 || ! document.table.setCents (key, cents))
    {
        status.setText ("Invalid offset. Keep the resulting pitch within MIDI notes 0-127.", juce::dontSendNotification);
        status.setColour (juce::Label::textColourId, juce::Colours::orange);
        const juce::Component::SafePointer<MicroTunePage> safe (this);
        juce::MessageManager::callAsync ([safe, key]
        {
            if (! safe) return;
            if (auto* cell = dynamic_cast<CentsCell*> (safe->table.getCellComponent (3, key)))
                cell->display (key, safe->processor.getLocalTuningInfo().document.table.pitch (key).value_or (key));
        });
        return;
    }
    processor.replaceLocalTuningDocument (document);
    // Refresh on the timer, after the edited label's callback has returned.
}

void MicroTunePage::selectedRowsChanged (int)
{
    if (tuningView) tuningView->selectionChanged();
    const auto count = table.getNumSelectedRows();
    selectionHint.setText (juce::String (count) + " selected - Shift/Command-click Key or Note to select.", juce::dontSendNotification);
    for (auto* button : { &setButton, &addButton, &resetButton }) button->setEnabled (count > 0);
}

void MicroTunePage::bulkEdit (bsii::microtuning::BulkEdit operation)
{
    const auto text = bulkCents.getText().trim();
    char* end = nullptr;
    const auto* start = text.toRawUTF8();
    const auto cents = operation == bsii::microtuning::BulkEdit::reset ? 0.0 : std::strtod (start, &end);
    std::vector<int> keys;
    const auto selected = table.getSelectedRows();
    for (int i = 0; i < selected.size(); ++i) keys.push_back (selected[i]);
    auto document = processor.getLocalTuningInfo().document;
    if ((operation != bsii::microtuning::BulkEdit::reset && (text.isEmpty() || end == start || *end != 0))
        || ! bsii::microtuning::editNotes (document.table, keys, operation, cents))
    {
        status.setText ("No notes changed. Select notes and keep every resulting pitch within 0-127.", juce::dontSendNotification);
        status.setColour (juce::Label::textColourId, juce::Colours::orange);
        return;
    }
    processor.replaceLocalTuningDocument (document);
    refresh();
}

void MicroTunePage::commitName()
{
    const auto name = nameEditor.getText().trim();
    auto document = processor.getLocalTuningInfo().document;
    if (name.isEmpty()) { nameEditor.setText (document.name, false); return; }
    if (name == document.name) return;
    document.name = name;
    processor.replaceLocalTuningDocument (document);
    refresh();
}

void MicroTunePage::confirmReplace (std::function<void()> action)
{
    if (! processor.getLocalTuningInfo().unsavedFile) { action(); return; }
    const juce::Component::SafePointer<MicroTunePage> safe (this);
    juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon)
        .withTitle ("Replace local table?")
        .withMessage ("This table has changes that have not been saved to a local table file.")
        .withButton ("Replace").withButton ("Cancel"),
        [safe, pendingAction = std::move (action)] (int result) { if (safe && result == 1) pendingAction(); });
}

void MicroTunePage::newTable()
{
    bsii::microtuning::Document document;
    document.targetTable = processor.getLocalTuningInfo().document.targetTable;
    processor.replaceLocalTuningDocument (document);
    refresh();
}

bool MicroTunePage::loadDocumentFile (const juce::File& file)
{
    const auto document = file.existsAsFile() && file.getSize() <= 65536
        ? bsii::microtuning::parseDocument (file.loadFileAsString()) : std::nullopt;
    if (! document || ! processor.replaceLocalTuningDocument (*document, false))
    {
        status.setText ("Could not load a valid MicroTune table. Current table kept.", juce::dontSendNotification);
        status.setColour (juce::Label::textColourId, juce::Colours::orange);
        return false;
    }
    refresh();
    return true;
}

bool MicroTunePage::saveDocumentFile (const juce::File& file)
{
    commitName();
    const auto document = bsii::microtuning::serialise (processor.getLocalTuningInfo().document);
    if (! document || ! file.replaceWithText (*document))
    {
        status.setText ("Could not save table file. Your local edits are still here.", juce::dontSendNotification);
        return false;
    }
    processor.markLocalTuningFileSaved();
    refresh();
    status.setText ("Saved local table: " + file.getFileName(), juce::dontSendNotification);
    return true;
}

void MicroTunePage::chooseLoad()
{
    chooser = std::make_unique<juce::FileChooser> ("Load a local MicroTune table", juce::File {}, "*.json");
    const juce::Component::SafePointer<MicroTunePage> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe] (const juce::FileChooser& selection)
        {
            if (! safe) return;
            const auto file = selection.getResult();
            if (file != juce::File {}) safe->loadDocumentFile (file);
            safe->chooser.reset();
        });
}

void MicroTunePage::chooseSave()
{
    chooser = std::make_unique<juce::FileChooser> ("Save a local MicroTune table",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("BSII Tuning.json"), "*.json");
    const juce::Component::SafePointer<MicroTunePage> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                          | juce::FileBrowserComponent::warnAboutOverwriting,
        [safe] (const juce::FileChooser& selection)
        {
            if (! safe) return;
            const auto file = selection.getResult();
            if (file != juce::File {}) safe->saveDocumentFile (file);
            safe->chooser.reset();
        });
}

void MicroTunePage::chooseScala()
{
    if (importDialog) { importDialog->toFront (true); return; }
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new ScalaImportPanel (processor));
    options.dialogTitle = "Import Tuning - local preview";
    options.dialogBackgroundColour = juce::Colour (0xff202326);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.componentToCentreAround = this;
    importDialog = options.launchAsync();
}
