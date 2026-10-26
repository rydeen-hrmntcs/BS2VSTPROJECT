#include "BSIIAfxPage.h"

AfxPage::AfxPage(BSIIControlAudioProcessor& p):processor(p)
{
    viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);
    for(auto* c:std::initializer_list<juce::Component*>{&heading,&hint,&source,&coverage,&detail,&status,&open,&save})content.addAndMakeVisible(c);
    heading.setText("AFX - local overlay archive",juce::dontSendNotification);
    heading.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    hint.setText("Inspect 25 key records locally. No audition, bank selection, hardware edits or saves.",juce::dontSendNotification);
    hint.setColour(juce::Label::textColourId,juce::Colours::lightblue);
    for(auto* label:{&hint,&coverage,&detail,&status})label->setMinimumHorizontalScale(1.0f);
    detail.setJustificationType(juce::Justification::topLeft);
    for(int i=0;i<25;++i)
    {
        auto& button=keys[static_cast<size_t>(i)];content.addAndMakeVisible(button);
        button.setComponentID("afx-key-"+juce::String(i));
        button.setClickingTogglesState(false);
        button.onClick=[this,i]{selectKey(i);};
    }
    open.onClick=[this]{chooseOpen();};save.onClick=[this]{chooseSave();};
    refresh();
}
AfxPage::~AfxPage(){chooser.reset();viewport.setViewedComponent(nullptr,false);}
void AfxPage::resized()
{
    viewport.setBounds(getLocalBounds());
    const int height=juce::jmax(450,getHeight());
    content.setSize(juce::jmax(1,getWidth()-(height>getHeight()?viewport.getScrollBarThickness():0)),height);
    auto area=content.getLocalBounds().reduced(12);
    heading.setBounds(area.removeFromTop(26));hint.setBounds(area.removeFromTop(38));
    auto actions=area.removeFromTop(32);open.setBounds(actions.removeFromLeft(175));actions.removeFromLeft(8);save.setBounds(actions.removeFromLeft(165));
    source.setBounds(area.removeFromTop(28));coverage.setBounds(area.removeFromTop(46));
    // Two rows of white keys with an upper black-key row for each octave.
    for(int octave=0;octave<2;++octave)
    {
        auto keyboard=area.removeFromTop(67).toFloat();
        const float width=keyboard.getWidth()/8.0f;
        constexpr int whitePosition[]{0,1,1,2,2,3,4,4,5,5,6,6};
        for(int pc=0;pc<12;++pc)
        {
            const int index=octave*12+pc;
            const bool black=pc==1||pc==3||pc==6||pc==8||pc==10;
            const auto x=keyboard.getX()+whitePosition[pc]*width-(black?width*0.3f:0);
            keys[static_cast<size_t>(index)].setBounds(juce::Rectangle<float>(x,keyboard.getY(),black?width*0.6f:width,black?32.0f:62.0f).toNearestInt());
        }
        if(octave==1)keys[24].setBounds(juce::Rectangle<float>(keyboard.getX()+7*width,keyboard.getY(),width,62).toNearestInt());
        for(int pc=0;pc<12;++pc)if(pc==1||pc==3||pc==6||pc==8||pc==10)keys[static_cast<size_t>(octave*12+pc)].toFront(false);
    }
    area.removeFromTop(8);detail.setBounds(area.removeFromTop(88));status.setBounds(area);
}
void AfxPage::refresh()
{
    const auto archive=processor.getLocalAfxArchive();
    const int selected=processor.getLocalAfxSelection();
    source.setText(archive?archive->sourceName()+" | "+juce::String(archive->bytes().getSize())+" bytes":"No archive open",juce::dontSendNotification);
    source.setTooltip(archive?"SHA-256: "+juce::SHA256(archive->bytes()).toHexString():juce::String());
    coverage.setText(archive?"Data: 25/25 | Names: "+juce::String(archive->nameCount())+"/25. Bank identity, protection and saved state are not verified.":"Open raw active-buffer replies. Names are optional; all 25 data records are required.",juce::dontSendNotification);
    save.setEnabled(archive!=nullptr);
    for(int i=0;i<25;++i)
    {
        auto& button=keys[static_cast<size_t>(i)];
        const auto note=juce::MidiMessage::getMidiNoteName(36+i,true,true,4); // Hardware labels C2..C4.
        const bool black=(i%12)==1||(i%12)==3||(i%12)==6||(i%12)==8||(i%12)==10;
        button.setButtonText(note);button.setEnabled(archive!=nullptr);
        button.setColour(juce::TextButton::buttonColourId,selected==i?juce::Colour(0xff177b99):black?juce::Colour(0xff24282c):juce::Colour(0xffd5dcdf));
        button.setColour(juce::TextButton::textColourOffId,selected==i||black?juce::Colours::white:juce::Colours::black);
        const auto* key=archive?archive->key(i):nullptr;
        button.setTooltip("Hardware key "+juce::String(i+1)+" (wire index "+juce::String(i)+")"+(key?" | "+key->displayName():""));
    }
    const auto* key=archive?archive->key(selected):nullptr;
    detail.setText(key?"Selected: Key "+juce::String(selected+1)+" ("+juce::MidiMessage::getMidiNoteName(36+selected,true,true,4)+"), wire index "+juce::String(selected)
        +"\n"+key->displayName()+" | "+(key->present?"Overlay present":"Empty overlay (underlying patch)")
        +"\nData preserved: 106 bytes. "+(key->name.isEmpty()?"No name reply in this archive.":"Name reply preserved: 26 bytes.")
        :"Select a key to inspect its record. Parameter editing and hardware transfer are not available yet.",juce::dontSendNotification);
}
void AfxPage::selectKey(int index){processor.selectLocalAfxKey(index);refresh();}
bool AfxPage::loadArchiveFile(const juce::File& file)
{
    juce::String error;auto archive=bsii::AfxArchive::readFile(file,error);
    if(!archive){status.setText(error,juce::dontSendNotification);return false;}
    processor.setLocalAfxArchive(std::move(archive));refresh();
    status.setText("Opened locally. Original bytes preserved; nothing sent to the synth.",juce::dontSendNotification);return true;
}
bool AfxPage::saveArchiveFile(const juce::File& file)
{
    const auto archive=processor.getLocalAfxArchive();
    if(!archive)return false;
    const auto& bytes=archive->bytes();
    const bool ok=file.replaceWithData(bytes.getData(),bytes.getSize());
    status.setText(ok?"Exact local copy saved. Not a verified hardware restoration file.":"Could not save the copy. Archive kept.",juce::dontSendNotification);return ok;
}
void AfxPage::chooseOpen()
{
    chooser=std::make_unique<juce::FileChooser>("Open AFX active-buffer replies",juce::File{},"*.syx");
    juce::Component::SafePointer<AfxPage> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& chosen)
    {if(safe){const auto file=chosen.getResult();if(file!=juce::File{})safe->loadArchiveFile(file);if(safe)safe->chooser.reset();}});
}
void AfxPage::chooseSave()
{
    chooser=std::make_unique<juce::FileChooser>("Save exact local AFX archive copy",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("AFX-active-buffer.syx"),"*.syx");
    juce::Component::SafePointer<AfxPage> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& chosen)
    {if(safe){auto file=chosen.getResult();if(file!=juce::File{})safe->saveArchiveFile(file.withFileExtension("syx"));if(safe)safe->chooser.reset();}});
}
