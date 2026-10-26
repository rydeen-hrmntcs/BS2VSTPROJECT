// Read active AFX records only. No selection, patch change, protection, import or save.
import Foundation
import CoreMIDI
import Darwin
func fail(_ text: String) -> Never { fputs(text + "\n",stderr); exit(1) }
guard CommandLine.arguments.count == 2 || (CommandLine.arguments.count == 3 && CommandLine.arguments[2] == "--data-only") else { fail("Usage: AfxRead EXISTING_EMPTY_OUTPUT_DIRECTORY [--data-only]") }
let dataOnly = CommandLine.arguments.count == 3
let directory = URL(fileURLWithPath:CommandLine.arguments[1],isDirectory:true)
let packetPath = directory.appendingPathComponent("packets.jsonl")
let messagePath = directory.appendingPathComponent("messages.jsonl")
try Data().write(to:packetPath,options:.withoutOverwriting)
try Data().write(to:messagePath,options:.withoutOverwriting)
let packets = try FileHandle(forWritingTo:packetPath), messages = try FileHandle(forWritingTo:messagePath)
defer { try? packets.close(); try? messages.close() }
let lock = NSLock(), ready = DispatchSemaphore(value:0)
var assembling = [UInt8](), response:[UInt8]?
var expectedKey = -1, expectedCommand = -1, expectedLength = 0
var logError = false, messageSerial = 0, responseSerial = 0
let start = Date()
func log(_ object:[String:Any],to handle:FileHandle) {
    do { var data=try JSONSerialization.data(withJSONObject:object,options:[.sortedKeys]);data.append(10);try handle.write(contentsOf:data) }
    catch { logError=true }
}
func name(_ endpoint:MIDIEndpointRef)->String {
    var text:Unmanaged<CFString>?
    MIDIObjectGetStringProperty(endpoint,kMIDIPropertyDisplayName,&text)
    return text?.takeRetainedValue() as String? ?? ""
}
var client=MIDIClientRef(), input=MIDIPortRef(), output=MIDIPortRef()
guard MIDIClientCreateWithBlock("BSII AFX read-only capture" as CFString,&client,nil)==noErr else { fail("Cannot create MIDI client") }
defer { if input != 0 {MIDIPortDispose(input)};if output != 0 {MIDIPortDispose(output)};MIDIClientDispose(client) }
guard MIDIInputPortCreateWithBlock(client,"AFX input" as CFString,&input,{list,_ in
    var packet=UnsafeRawPointer(list).advanced(by:MemoryLayout<MIDIPacketList>.offset(of:\.packet)!).assumingMemoryBound(to:MIDIPacket.self)
    for _ in 0..<list.pointee.numPackets {
        let p=UnsafeRawPointer(packet).advanced(by:MemoryLayout<MIDIPacket>.offset(of:\.data)!).assumingMemoryBound(to:UInt8.self)
        let bytes=Array(UnsafeBufferPointer(start:p,count:Int(packet.pointee.length)))
        lock.lock()
        log(["seconds":Date().timeIntervalSince(start),"bytes":bytes],to:packets)
        for byte in bytes {
            if byte>=0xf8 {continue}
            if byte==0xf0 {assembling=[byte];continue}
            if assembling.isEmpty {continue}
            assembling.append(byte)
            if byte==0xf7 {
                messageSerial+=1
                log(["seconds":Date().timeIntervalSince(start),"serial":messageSerial,"bytes":assembling],to:messages)
                let prefix:[UInt8]=[0xf0,0,0x20,0x29,0,0x33,0]
                if assembling.count==expectedLength && Array(assembling.prefix(7))==prefix
                    && Int(assembling[7])==expectedCommand && Int(assembling[8])==expectedKey
                    && assembling.dropFirst().dropLast().allSatisfy({$0<128}) {
                    response=assembling;responseSerial=messageSerial;ready.signal()
                }
                assembling.removeAll()
            } else if byte>=0x80 || assembling.count>4096 {assembling.removeAll()}
        }
        lock.unlock();packet=UnsafePointer(MIDIPacketNext(packet))
    }
})==noErr else {fail("Cannot create input")}
let sources=(0..<MIDIGetNumberOfSources()).map{MIDIGetSource($0)}.filter{name($0).lowercased().contains("bass station")}
let destinations=(0..<MIDIGetNumberOfDestinations()).map{MIDIGetDestination($0)}.filter{name($0).lowercased().contains("bass station")}
guard sources.count==1 && destinations.count==1 else {fail("Expected exactly one BSII input and output")}
guard MIDIPortConnectSource(input,sources[0],nil)==noErr && MIDIOutputPortCreate(client,"AFX read output" as CFString,&output)==noErr else {fail("Cannot connect ports")}
var archive=Data(), requests=[[String:Any]]()
var stopped=false
for kind in (dataOnly ? ["data"] : ["data","name"]) {
    for key in 0..<25 {
        let requestCommand:UInt8=kind=="data" ? 0x4f:0x54
        lock.lock();expectedKey=key;expectedCommand=kind=="data" ? 0x4e:0x53;expectedLength=kind=="data" ? 106:26;response=nil;lock.unlock()
        while ready.wait(timeout:.now()) == .success {}
        let bytes:[UInt8]=[0xf0,0,0x20,0x29,0,0x33,0,requestCommand,UInt8(key),0xf7]
        let storage=UnsafeMutableRawPointer.allocate(byteCount:1024,alignment:8)
        let list=storage.bindMemory(to:MIDIPacketList.self,capacity:1)
        let first=MIDIPacketListInit(list)
        bytes.withUnsafeBufferPointer{_ = MIDIPacketListAdd(list,1024,first,0,$0.count,$0.baseAddress!)}
        let sent=MIDISend(output,destinations[0],list);storage.deallocate()
        var record:[String:Any]=["kind":kind,"key":key,"requestBytes":bytes,"sentSeconds":Date().timeIntervalSince(start)]
        if sent != noErr {record["error"]="dispatch-failed";requests.append(record);stopped=true;break}
        let received=ready.wait(timeout:.now()+3) == .success
        lock.lock();let captured=response;let serial=responseSerial;let failed=logError;lock.unlock()
        if received,let captured=captured,!failed {
            archive.append(contentsOf:captured);record["responseBytes"]=captured;record["messageSerial"]=serial
        } else {record["error"]=failed ? "log-io-error":"no-matching-reply";stopped=true}
        requests.append(record)
        if stopped {break}
        Thread.sleep(forTimeInterval:0.1)
    }
    if stopped {break}
}
try archive.write(to:directory.appendingPathComponent("active-buffer-replies.syx"),options:.withoutOverwriting)
let report:[String:Any]=["schemaVersion":1,"scope":"active-buffer","input":name(sources[0]),"output":name(destinations[0]),"firmware":NSNull(),"bankQueried":false,"protectionQueried":false,"bankSelectionSent":false,"writesSent":false,"stoppedEarly":stopped,"namesRequested":!dataOnly,"requests":requests]
try JSONSerialization.data(withJSONObject:report,options:[.prettyPrinted,.sortedKeys]).write(to:directory.appendingPathComponent("transport.json"),options:.withoutOverwriting)
print("Read-only AFX capture: \(requests.count) requests; \(archive.count) response bytes; partial=\(stopped). Raw packet/message logs retained.")
