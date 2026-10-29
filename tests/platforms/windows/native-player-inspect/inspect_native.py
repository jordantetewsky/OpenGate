"""Read-only Qt 5 x64 metadata / PE inspection; never loads Resolve code."""
import argparse
import bisect
import json
from pathlib import Path
import struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

parser = argparse.ArgumentParser()
parser.add_argument('binary')
parser.add_argument('--class-name', default='UiAudioPanelImp')
parser.add_argument('--disasm', type=lambda s: int(s, 0))
parser.add_argument('--size', type=lambda s: int(s, 0), default=0x800)
parser.add_argument('--refs', type=lambda s: int(s, 0))
args = parser.parse_args()
pe = pefile.PE(args.binary, fast_load=True)
data = Path(args.binary).read_bytes()
base = pe.OPTIONAL_HEADER.ImageBase
def off(va): return pe.get_offset_from_rva(va-base)
def va(offset): return base+pe.get_rva_from_offset(offset)
def unpack(fmt, offset): return struct.unpack_from(fmt, data, offset)
def occurrences(needle):
    p = data.find(needle)
    while p >= 0:
        yield p
        p = data.find(needle, p+1)

if args.refs:
    exception=pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    functions=[unpack('<III',pe.get_offset_from_rva(exception.VirtualAddress)+i)
        for i in range(0,exception.Size,12)]
    starts=[x[0] for x in functions]
    results=[]
    text_section=next(s for s in pe.sections if s.Name.startswith(b'.text'))
    lo=text_section.PointerToRawData
    hi=lo+text_section.SizeOfRawData
    import re
    for match in re.finditer(b'\xe8|\xe9|\xff[\x15\x25]|[\x48\x4c]\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]',data[lo:hi]):
        p=lo+match.start()
        n=len(match.group())+4
        instruction=base+text_section.VirtualAddress+p-lo
        target=instruction+n+unpack('<i',p+n-4)[0]
        if target!=args.refs: continue
        index=bisect.bisect_right(starts,instruction-base)-1
        begin,end,_=functions[index]
        results.append(dict(instruction=hex(instruction),function=hex(base+begin),
            size=hex(end-begin),pattern=match.group().hex()))
    print(json.dumps(results,indent=2))
elif args.disasm:
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT'],
        pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT']])
    imports = {imp.address: f'{entry.dll.decode()}!{(imp.name or b"ordinal").decode()}'
               for entry in list(getattr(pe,'DIRECTORY_ENTRY_IMPORT',[]))+list(getattr(pe,'DIRECTORY_ENTRY_DELAY_IMPORT',[]))
               for imp in entry.imports}
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    for ins in decoder.disasm(data[off(args.disasm):off(args.disasm)+args.size], args.disasm):
        annotations=[]
        for operand in ins.operands:
            if operand.type == 3 and ins.reg_name(operand.mem.base) == 'rip':
                address=ins.address+ins.size+operand.mem.disp
                if address in imports: annotations.append(imports[address])
                else:
                    try:
                        target=unpack('<Q',off(address))[0]
                        if 0 < target < pe.OPTIONAL_HEADER.SizeOfImage:
                            pos=pe.get_offset_from_rva(target)+2
                            name=data[pos:data.find(b'\0',pos,pos+600)]
                            if name and name.startswith(b'?') and all(32<=c<127 for c in name):
                                annotations.append('import-name: '+name.decode())
                    except (ValueError,struct.error): pass
        print(f'{ins.address:#x}: {ins.mnemonic:8} {ins.op_str:45} {"; ".join(annotations)}')
else:
    name=args.class_name.encode()
    starts=set(occurrences(name+b'\0'))
    # Qt 5 QByteArrayData: ref, size, alloc/capacity, padding, relative offset.
    prefix=struct.pack('<iIII', -1, len(name), 0, 0)
    results=[]
    for p in occurrences(prefix):
        delta=unpack('<q',p+16)[0]
        if p+delta not in starts: continue
        def string(index):
            item=p+index*24
            length=unpack('<i',item+4)[0]
            start=item+unpack('<q',item+16)[0]
            return data[start:start+length].decode(errors='replace')
        for ref in occurrences(struct.pack('<Q',va(p))):
            try:
                supermeta,stringptr,metadataptr,callptr=unpack('<QQQQ',ref-8)
                m=off(metadataptr)
                header=unpack('<14I',m)
                if header[0] not in (7,8) or header[1]!=0 or header[4]>2000: continue
                methods=[]
                for i in range(header[4]):
                    ni,argc,params,tag,flags=unpack('<5I',m+(header[5]+i*5)*4)
                    types=unpack('<'+'I'*(argc+1),m+params*4)
                    names=unpack('<'+'I'*argc,m+(params+argc+1)*4)
                    methods.append(dict(index=i,name=string(ni),flags=hex(flags),
                        types=[string(t&0x7fffffff) if t&0x80000000 else t for t in types],
                        parameters=[string(n) for n in names]))
                results.append(dict(binary=args.binary,base=hex(base),class_name=args.class_name,
                    metaobject=hex(va(ref-8)),strings=hex(va(p)),metadata=hex(metadataptr),
                    static_metacall=hex(callptr),signal_count=header[13],methods=methods))
            except (ValueError,struct.error,OverflowError): pass
    print(json.dumps(results,indent=2))
