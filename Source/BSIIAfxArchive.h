#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <memory>

namespace bsii
{
// Local, opaque active-buffer archive. No commands, port access or write eligibility.
class AfxArchive
{
public:
    static constexpr int keyCount=25;
    static constexpr int maxBytes=keyCount*(106+26);
    struct Key
    {
        juce::MemoryBlock data, name;
        bool present=false;
        juce::String displayName() const
        {
            if(name.isEmpty()) return "Name unavailable";
            const auto* b=static_cast<const juce::uint8*>(name.getData());
            juce::String text;
            for(int i=9;i<25;++i) if(b[i]>=32 && b[i]<=125) text+=juce::String::charToString(static_cast<juce::juce_wchar>(b[i]));
            return text.trim().isEmpty()?"(empty name)":text.trim();
        }
    };
    static std::shared_ptr<const AfxArchive> parse(const juce::MemoryBlock& input,const juce::String& source,juce::String& error)
    {
        error.clear();
        const auto reject=[&](const char* text)->std::shared_ptr<const AfxArchive>{error=text;return {};};
        if(input.isEmpty() || input.getSize()>maxBytes) return reject("Choose one AFX archive with 25 data records and optional names (up to 3300 bytes).");
        auto archive=std::shared_ptr<AfxArchive>(new AfxArchive());
        const auto* bytes=static_cast<const juce::uint8*>(input.getData());
        size_t offset=0;
        while(offset<input.getSize())
        {
            const auto* b=bytes+offset;
            const juce::uint8 prefix[]{0xf0,0,0x20,0x29,0,0x33,0};
            if(input.getSize()-offset<10 || !std::equal(std::begin(prefix),std::end(prefix),b) || b[8]>=25)
                return reject("Unsupported AFX message or key. Previous archive kept.");
            const int length=b[7]==0x4e?106:b[7]==0x53?26:0;
            if(!length || input.getSize()-offset<static_cast<size_t>(length) || b[length-1]!=0xf7)
                return reject("Expected exact AFX data/name replies. Clear markers and other commands are unsupported.");
            for(int i=1;i<length-1;++i) if(b[i]>=128) return reject("Malformed AFX bytes. Previous archive kept.");
            auto& key=archive->keys[b[8]];
            auto& record=b[7]==0x4e?key.data:key.name;
            if(!record.isEmpty()) return reject("Duplicate AFX key record. Previous archive kept.");
            record=juce::MemoryBlock(b,static_cast<size_t>(length));
            if(b[7]==0x4e)
            {
                // First decoded byte from Components' contiguous 7-bit packing.
                key.present=(((static_cast<unsigned>(b[9])<<1)&255u)|(b[10]>>6))==0;
            }
            else ++archive->names;
            offset+=static_cast<size_t>(length);
        }
        for(const auto& key:archive->keys) if(key.data.isEmpty()) return reject("Incomplete AFX data: all 25 key records are required. Raw partial capture is kept separately.");
        archive->raw=input;archive->source=source;
        return archive;
    }
    static std::shared_ptr<const AfxArchive> readFile(const juce::File& file,juce::String& error)
    {
        juce::FileInputStream input(file);
        if(!input.openedOk()){error="Could not open AFX archive. Previous archive kept.";return {};}
        juce::MemoryBlock bytes;input.readIntoMemoryBlock(bytes,maxBytes+1);
        return parse(bytes,file.getFileName(),error);
    }
    const Key* key(int index) const {return juce::isPositiveAndBelow(index,keyCount)?&keys[static_cast<size_t>(index)]:nullptr;}
    const juce::MemoryBlock& bytes() const {return raw;}
    const juce::String& sourceName() const {return source;}
    int nameCount() const {return names;}
private:
    std::array<Key,keyCount> keys;
    juce::MemoryBlock raw;
    juce::String source;
    int names=0;
};
}
