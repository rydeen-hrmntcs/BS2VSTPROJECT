#pragma once
#include "BSIIProjectRecall.h"
#include <array>
#include <algorithm>
#include <memory>

namespace bsii
{
// Strict modern bank profile. Stored packets require the separate explicit write workflow.
class PatchBank
{
public:
    static constexpr size_t byteSize = 128 * 154;
    static std::shared_ptr<const PatchBank> parse (const juce::MemoryBlock& data,
                                                  const juce::String& source, juce::String& error)
    {
        error.clear();
        if (data.getSize() != byteSize)
        { error = "Choose a complete 128-patch Bass Station II bank (.syx)."; return {}; }
        auto bank = std::shared_ptr<PatchBank> (new PatchBank());
        std::array<bool,128> seen {};
        const auto* bytes = static_cast<const juce::uint8*> (data.getData());
        for (size_t i = 0; i < 128; ++i)
        {
            const auto* m = bytes + i * 154;
            const juce::uint8 prefix[] {0xf0,0,0x20,0x29,0,0x33};
            bool valid = std::equal (std::begin(prefix),std::end(prefix),m) && m[153] == 0xf7;
            for (int b = 1; b < 153; ++b) valid &= m[b] < 128;
            valid &= m[6] == 0 && m[7] == 1 && m[9] == 0 && m[10] == 0 && m[11] == 0 && m[12] == 0;
            if (! valid || m[8] > 127 || seen[m[8]])
            { error = "Bank has an unsupported message or duplicate entry. Previous bank kept."; return {}; }
            seen[m[8]] = true;
            bank->entries[m[8]] = juce::MemoryBlock (m,154);
        }
        // 128 unique 7-bit indices imply the complete 0..127 set.
        for (int i = 0; i < 128; ++i)
        {
            if (! isRecallPatch (bank->temporaryPatch(i)))
            { error = "Bank entry cannot be loaded as a temporary patch. Previous bank kept."; return {}; }
            bank->names[static_cast<size_t>(i)] = decodePatchFields(bank->entries[static_cast<size_t>(i)]).patchName;
        }
        bank->sourceName = source;
        return bank;
    }
    static std::shared_ptr<const PatchBank> readFile (const juce::File& file, juce::String& error)
    {
        juce::FileInputStream input(file);
        if (! input.openedOk()) { error = "Could not open bank. Previous bank kept."; return {}; }
        juce::MemoryBlock data;
        input.readIntoMemoryBlock(data,static_cast<ssize_t>(byteSize+1));
        return parse(data,file.getFileName(),error);
    }
    juce::MemoryBlock temporaryPatch (int index) const
    {
        if (! juce::isPositiveAndBelow(index,128)) return {};
        auto patch = entries[static_cast<size_t>(index)];
        auto* bytes = static_cast<juce::uint8*>(patch.getData());
        if (patch.getSize() == 154) std::fill(bytes+6,bytes+13,0);
        return patch;
    }
    juce::MemoryBlock storedPatch (int index, int destination) const
    {
        if (! juce::isPositiveAndBelow(index,128) || ! juce::isPositiveAndBelow(destination,128)) return {};
        auto patch = entries[static_cast<size_t>(index)];
        static_cast<juce::uint8*>(patch.getData())[8] = static_cast<juce::uint8>(destination);
        return patch; // All 144 payload bytes at offsets 9..152 are preserved.
    }
    juce::String name (int index) const
    { return juce::isPositiveAndBelow(index,128) ? names[static_cast<size_t>(index)] : juce::String(); }
    const juce::String& source() const { return sourceName; }
private:
    PatchBank() = default;
    std::array<juce::MemoryBlock,128> entries;
    std::array<juce::String,128> names;
    juce::String sourceName;
};
}
