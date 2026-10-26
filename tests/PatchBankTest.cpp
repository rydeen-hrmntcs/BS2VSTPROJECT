#include "BSIIPatchBank.h"
#include "BSIIStoredPatchWrite.h"
#include <iostream>
#include <cstring>
int main(int argc,char** argv)
{
    if (argc!=2) return 2;
    int failures=0;
    auto check=[&](bool ok,const char* text){ if(!ok){++failures;std::cerr<<"FAIL: "<<text<<'\n';} };
    juce::MemoryBlock fixture;
    if (!juce::File(juce::String::fromUTF8(argv[1])).loadFileAsData(fixture) || !bsii::isRecallPatch(fixture)) return 2;
    std::array<juce::MemoryBlock,128> entries;
    juce::MemoryBlock raw;
    for (int i=127;i>=0;--i)
    {
        auto patch=fixture;auto* b=static_cast<juce::uint8*>(patch.getData());
        b[7]=1;b[8]=static_cast<juce::uint8>(i);b[20]=static_cast<juce::uint8>(i);
        std::fill(b+137,b+153,' ');
        const auto name="Entry "+juce::String(i);
        std::memcpy(b+137,name.toRawUTF8(),static_cast<size_t>(name.getNumBytesAsUTF8()));
        entries[static_cast<size_t>(i)]=patch;raw.append(patch.getData(),patch.getSize());
    }
    juce::String error;
    auto bank=bsii::PatchBank::parse(raw,"synthetic.syx",error);
    check(bank!=nullptr && error.isEmpty(),"complete reordered bank accepted");
    if (!bank) return 1;
    check(bank->source()=="synthetic.syx","source label retained");
    for(int i=0;i<128;++i)
    {
        auto expected=entries[static_cast<size_t>(i)];auto* b=static_cast<juce::uint8*>(expected.getData());
        std::fill(b+6,b+13,0);
        check(bank->temporaryPatch(i)==expected,"full payload preserved with exact temporary header");
        check(bsii::isRecallPatch(bank->temporaryPatch(i)),"every extracted entry passes production gate");
        check(bank->name(i)=="Entry "+juce::String(i),"names indexed independently of stream order");
    }
    check(bank->temporaryPatch(-1).isEmpty() && bank->temporaryPatch(128).isEmpty(),"invalid indices rejected");
    auto reject=[&](const juce::MemoryBlock& data){check(!bsii::PatchBank::parse(data,"bad",error) && error.isNotEmpty(),"bad bank rejects atomically");};
    reject(juce::MemoryBlock());reject(fixture);reject(juce::MemoryBlock(raw.getData(),raw.getSize()-1));
    auto bad=raw;const juce::uint8 extra=0;bad.append(&extra,1);reject(bad);
    for(const auto offset : {0,5,6,7,9,10,11,12,20,153})
    {
        bad=raw;auto* b=static_cast<juce::uint8*>(bad.getData());b[offset]=static_cast<juce::uint8>(offset==0?0xf1:offset==5?0x34:offset==7||offset==153?0:offset==20?0x80:1);reject(bad);
    }
    bad=raw;static_cast<juce::uint8*>(bad.getData())[8]=126;reject(bad); // Duplicate index.
    bad=raw;static_cast<juce::uint8*>(bad.getData())[8]=128;reject(bad);
    // Exercise transactions against an independent synthetic hardware bank.
    {
        using Write=bsii::StoredPatchWrite;
        struct Harness
        {
            Write write;
            std::array<juce::MemoryBlock,128> hardware;
            juce::MemoryBlock current,reply,savedBefore,savedAfter;
            uint64_t serial=0;
            double now=0;
            int writes=0, requests=0;
            bool saveOk=true, answer=true, corruptNeighbour=false, failAfter=false;
            juce::String device="virtual-BSII";
            void tick()
            {
                now+=101;
                write.tick(now,device,reply,serial,[&](const juce::MemoryBlock& packet)
                {
                    const auto* b=static_cast<const juce::uint8*>(packet.getData());
                    if(packet.getSize()==154)
                    {
                        if(!Write::isStoredReply(packet,1)) return false;
                        ++writes;hardware[1]=packet;
                        if(corruptNeighbour) static_cast<juce::uint8*>(hardware[2].getData())[20]=100;
                    }
                    else
                    {
                        ++requests;
                        if(answer)
                        {
                            reply=packet.getSize()==9 ? current : hardware[b[8]];
                            ++serial;
                        }
                    }
                    return true;
                },[&](const juce::MemoryBlock& before,const juce::MemoryBlock& original,const juce::MemoryBlock& planned,const juce::MemoryBlock& temporary)
                {
                    savedBefore=before;
                    return saveOk && original==hardware[1] && Write::isStoredReply(planned,1) && temporary==current;
                },[&](const juce::MemoryBlock& after){ savedAfter=after;return !failAfter; });
            }
            void finish(){ for(int n=0;n<1200 && write.busy();++n) tick(); }
        };
        const auto make=[&]
        {
            Harness h;h.hardware=entries;h.current=fixture;return h;
        };
        auto h=make();
        check(!h.write.begin(*bank,11,128,h.device,0) && !h.write.begin(*bank,11,1,{},0),"invalid plan rejects without output");
        check(h.write.begin(*bank,11,1,h.device,0),"explicit read-only preparation starts");
        h.finish();
        check(h.write.ready() && h.writes==0 && h.requests==129 && h.savedBefore.getSize()==19712,"preparation backs up sound and all slots without writing");
        for(int n=0;n<10;++n)h.tick();
        check(h.writes==0,"ready preview never writes by itself");
        check(h.write.confirm(h.now),"explicit confirmation starts fresh destination recheck");h.finish();
        check(h.write.phase()==Write::Phase::complete && h.writes==1 && h.requests==258,"exactly one write plus independent all-slot readback");
        check(h.hardware[1]==bank->storedPatch(11,1) && h.hardware[2]==entries[2],"destination changed and neighbour preserved");
        check(h.savedAfter.getSize()==19712 && h.write.originalPatch()==entries[1],"verification evidence and original retained");
        check(!h.write.confirm(h.now),"completed transaction cannot replay on confirm");
        auto failed=make();failed.saveOk=false;failed.write.begin(*bank,11,1,failed.device,0);failed.finish();
        check(failed.write.phase()==Write::Phase::failed && failed.writes==0,"backup failure prevents writes");
        auto stale=make();stale.write.begin(*bank,11,1,stale.device,0);stale.finish();
        static_cast<juce::uint8*>(stale.hardware[1].getData())[20]=99;
        stale.write.confirm(stale.now);stale.finish();
        check(stale.write.phase()==Write::Phase::failed && stale.writes==0,"changed destination aborts before write");
        auto disconnected=make();disconnected.write.begin(*bank,11,1,disconnected.device,0);disconnected.finish();
        disconnected.device="another-device";disconnected.tick();
        check(!disconnected.write.confirm(disconnected.now) && disconnected.writes==0,"connection change invalidates ready plan");
        auto timeout=make();timeout.answer=false;timeout.write.begin(*bank,11,1,timeout.device,0);timeout.finish();
        check(timeout.write.phase()==Write::Phase::failed && timeout.writes==0 && timeout.requests==1,"missing reply times out without retries");
        auto mismatch=make();mismatch.corruptNeighbour=true;mismatch.write.begin(*bank,11,1,mismatch.device,0);mismatch.finish();
        mismatch.write.confirm(mismatch.now);mismatch.finish();
        check(mismatch.write.phase()==Write::Phase::failed && mismatch.writes==1 && mismatch.savedAfter.getSize()==19712,"neighbour mismatch retains evidence and never retries or auto-rolls back");
        auto lost=make();lost.write.begin(*bank,11,1,lost.device,0);lost.finish();lost.write.confirm(lost.now);
        for(int n=0;n<50 && !lost.write.wrote();++n)lost.tick();
        lost.answer=false;lost.finish();
        check(lost.write.phase()==Write::Phase::failed && lost.writes==1,"lost verification after dispatch never retries or automatically restores");
        auto oldReply=make();oldReply.answer=false;oldReply.reply=fixture;oldReply.serial=5;
        oldReply.write.begin(*bank,11,1,oldReply.device,0);oldReply.finish();
        check(oldReply.write.phase()==Write::Phase::failed && oldReply.writes==0,"stale matching current reply cannot satisfy a fresh request");
        auto expired=make();expired.write.begin(*bank,11,1,expired.device,0);expired.finish();
        check(!expired.write.confirm(expired.now+120001) && expired.writes==0,"expired confirmation cannot write");
        auto cancelled=make();cancelled.write.begin(*bank,11,1,cancelled.device,0);cancelled.write.cancel("cancelled");cancelled.tick();
        check(cancelled.writes==0 && cancelled.requests==0,"cancellation before first tick stays silent");
        check(Write::request(-1).isEmpty() && Write::request(128).isEmpty(),"invalid request index rejected");
        check(!Write::isStoredReply(fixture,1) && !Write::isStoredReply(entries[2],1),"temporary and wrong-index packets cannot verify a stored write");
    }
    const auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("bsii-bank-test",".syx");
    check(file.replaceWithData(raw.getData(),raw.getSize()),"test file created");
    check(bsii::PatchBank::readFile(file,error)!=nullptr,"valid file read");
    bad=raw;bad.append(&extra,1);check(file.replaceWithData(bad.getData(),bad.getSize()),"oversize fixture written");
    check(!bsii::PatchBank::readFile(file,error),"bounded read detects trailing data");
    file.deleteFile();
    check(!bsii::PatchBank::readFile(file,error),"missing file rejected");
    std::cout<<(failures==0?"PASS":"FAIL")<<" strict bank parsing and temporary-patch extraction ("<<failures<<" failures)\n";
    return failures==0?0:1;
}
