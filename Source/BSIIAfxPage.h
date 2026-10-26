#pragma once
#include "PluginProcessor.h"

class AfxPage : public juce::Component
{
public:
    explicit AfxPage(BSIIControlAudioProcessor&);
    ~AfxPage() override;
    void resized() override;
    bool loadArchiveFile(const juce::File&);
    bool saveArchiveFile(const juce::File&);
    void selectKey(int);
private:
    void refresh();
    void chooseOpen();
    void chooseSave();
    BSIIControlAudioProcessor& processor;
    juce::Viewport viewport;
    juce::Component content;
    juce::Label heading,hint,source,coverage,detail,status;
    juce::TextButton open{"Open AFX archive..."},save{"Save exact copy..."};
    std::array<juce::TextButton,25> keys;
    std::unique_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AfxPage)
};
