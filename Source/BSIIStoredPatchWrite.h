#pragma once
#include "BSIIPatchBank.h"
#include <functional>
#include <cstring>

namespace bsii
{
// Message-thread transaction. No port, file, editor or host-state ownership.
// Only confirm() permits a stored write; preparing is entirely read-only.
class StoredPatchWrite
{
public:
    enum class Phase { idle, capturing, backingUp, ready, checking, settling, verifying, complete, failed };
    using Send = std::function<bool(const juce::MemoryBlock&)>;
    using SaveBefore = std::function<bool(const juce::MemoryBlock&, const juce::MemoryBlock&, const juce::MemoryBlock&, const juce::MemoryBlock&)>;
    using SaveAfter = std::function<bool(const juce::MemoryBlock&)>;
    Phase phase() const { return state; }
    bool busy() const { return state==Phase::capturing || state==Phase::backingUp || state==Phase::checking || state==Phase::settling || state==Phase::verifying; }
    bool ready() const { return state==Phase::ready; }
    bool wrote() const { return dispatched; }
    uint64_t token() const { return generation; }
    const juce::MemoryBlock& temporaryPatch() const { return temporary; }
    int destination() const { return target; }
    int sourceIndex() const { return entry; }
    const juce::String& sourceName() const { return source; }
    const juce::String& status() const { return message; }
    const juce::MemoryBlock& originalPatch() const { return original; }
    const juce::MemoryBlock& plannedPatch() const { return planned; }
    const juce::MemoryBlock& originalBank() const { return before; }
    int progress() const { return index; }

    bool begin(const PatchBank& bank, int selected, int destination, const juce::String& device, double now)
    {
        if (busy()) return false;
        auto packet=bank.storedPatch(selected,destination);
        if (packet.isEmpty() || device.isEmpty()) return false;
        ++generation;
        state=Phase::capturing; source=bank.source(); entry=selected; target=destination;
        connection=device; planned=std::move(packet); original.reset(); before.reset(); collected.reset(); temporary.reset();
        dispatched=false; waiting=false; index=0; nextAt=now;
        message="Backing up the current temporary sound...";
        return true;
    }
    bool confirm(double now)
    {
        if (!ready()) return false;
        if (now-readyAt>120000.0) { stop("Preview expired. Prepare a fresh backup."); return false; }
        state=Phase::checking; index=target; waiting=false; nextAt=now;
        message="Checking the destination still matches its backup...";
        return true;
    }
    void cancel(const juce::String& reason)
    { if (busy() || ready()) stop(reason); }

    static bool isStoredReply(const juce::MemoryBlock& data, int slot)
    {
        if (data.getSize()!=154 || !juce::isPositiveAndBelow(slot,128)) return false;
        const auto* b=static_cast<const juce::uint8*>(data.getData());
        const juce::uint8 prefix[]{0xf0,0,0x20,0x29,0,0x33,0,1};
        if (!std::equal(std::begin(prefix),std::end(prefix),b) || b[8]!=slot || b[153]!=0xf7) return false;
        for (int i=1;i<153;++i) if(b[i]>=128) return false;
        // Only the modern export profile already accepted by PatchBank is supported.
        for (int i=9;i<=12;++i) if(b[i]!=0) return false;
        return true;
    }
    static juce::MemoryBlock request(int slot)
    {
        if (!juce::isPositiveAndBelow(slot,128)) return {};
        const juce::uint8 bytes[]{0xf0,0,0x20,0x29,0,0x33,0,0x41,static_cast<juce::uint8>(slot),0xf7};
        return {bytes,sizeof(bytes)};
    }

    void tick(double now, const juce::String& device, const juce::MemoryBlock& reply, uint64_t serial,
              const Send& send, const SaveBefore& saveBefore, const SaveAfter& saveAfter)
    {
        if (!busy() && !ready()) return;
        if (device.isEmpty() || device!=connection) { stop("MIDI connection changed. Prepare again; no automatic retry."); return; }
        if (ready())
        {
            if (now-readyAt>120000.0) stop("Preview expired. Prepare a fresh backup.");
            return;
        }
        if (state==Phase::settling)
        {
            if(now<nextAt) return;
            state=Phase::verifying; index=0; collected.reset(); waiting=false;
            message="Reading all 128 presets to verify the write...";
        }
        if(waiting)
        {
            if(serial>afterSerial && (state==Phase::capturing ? isRecallPatch(reply) : isStoredReply(reply,index)))
            {
                waiting=false;
                if(state==Phase::capturing)
                {
                    temporary=reply; state=Phase::backingUp; index=0; nextAt=now+100.0;
                    message="Reading all 128 stored presets before writing...";
                    return;
                }
                if(state==Phase::checking)
                {
                    if(reply!=original) { stop("Destination changed since backup. Nothing was written; prepare again."); return; }
                    // Recheck the exact packet at the sole destructive dispatch point.
                    if(!isStoredReply(planned,target) || !send(planned)) { stop("Stored write could not be dispatched."); return; }
                    dispatched=true; state=Phase::settling; nextAt=now+500.0;
                    message="Write sent. Waiting for independent stored readback...";
                    return;
                }
                collected.append(reply.getData(),reply.getSize());
                ++index; nextAt=now+100.0;
                if(index==128)
                {
                    if(state==Phase::backingUp)
                    {
                        juce::String error;
                        auto bank=PatchBank::parse(collected,"hardware backup",error);
                        if(!bank) { stop("Hardware bank profile is unsupported. Nothing was written."); return; }
                        before=collected; original=bank->storedPatch(target,target);
                        if(!saveBefore(before,original,planned,temporary)) { stop("Could not save and verify the backup files. Nothing was written."); return; }
                        state=Phase::ready; readyAt=now;
                        message="Backup saved. Review source and destination, then explicitly confirm the write.";
                    }
                    else
                    {
                        bool matches=collected.getSize()==before.getSize();
                        const auto* bytes=static_cast<const juce::uint8*>(collected.getData());
                        const auto* baseline=static_cast<const juce::uint8*>(before.getData());
                        for(int slot=0;matches && slot<128;++slot)
                        {
                            const auto* expected=slot==target ? planned.getData() : baseline+slot*154;
                            matches=std::memcmp(bytes+slot*154,expected,154)==0;
                        }
                        const bool saved=saveAfter(collected);
                        if(!matches) { stop("Stored readback differs. Stop: no automatic retry or rollback. Keep the backup."); return; }
                        if(!saved) { stop("Readback matched, but its report could not be saved. Keep the backup."); return; }
                        state=Phase::complete;
                        message="Stored preset "+juce::String(target)+" verified. Other 127 presets unchanged. Backup retained.";
                    }
                    return;
                }
            }
            else if(now>=deadline) { stop("No matching stored reply. Stop and keep the backup; no automatic retry."); return; }
            else return;
        }
        if(!waiting && now>=nextAt)
        {
            afterSerial=serial;
            const juce::uint8 current[]{0xf0,0,0x20,0x29,0,0x33,0,0x40,0xf7};
            const auto packet=state==Phase::capturing ? juce::MemoryBlock(current,sizeof(current)) : request(index);
            if(!send(packet)) { stop("MIDI request could not be sent. Keep the backup."); return; }
            waiting=true; deadline=now+3000.0;
        }
    }
private:
    void stop(const juce::String& text)
    {
        state=Phase::failed; waiting=false;
        message=text+(dispatched ? " A write was sent; storage is not fully verified." : "");
    }
    Phase state=Phase::idle;
    juce::String source,connection,message{"Open a bank to prepare a stored-preset write."};
    juce::MemoryBlock planned,original,before,collected,temporary;
    int entry=-1,target=-1,index=0;
    bool waiting=false,dispatched=false;
    uint64_t afterSerial=0,generation=0;
    double nextAt=0,deadline=0,readyAt=0;
};
}
