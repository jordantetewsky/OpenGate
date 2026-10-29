#!/usr/bin/env python3
"""Read-only, bounded audit of the observed Resolve keyboard format.
Not a runtime binding provider; no preference writes or native registration.
"""
import argparse, collections, json, struct
from pathlib import Path
import xml.etree.ElementTree as ET

LIMIT=16*1024*1024
class InvalidPreset(ValueError): pass
class Reader:
    def __init__(self,data): self.data=data; self.pos=0
    def take(self,n):
        if n<0 or n>LIMIT or self.pos+n>len(self.data): raise InvalidPreset('truncated or oversized field')
        value=self.data[self.pos:self.pos+n];self.pos+=n;return value
    def integer(self): return struct.unpack('>I',self.take(4))[0]
    def count(self,maximum):
        n=self.integer()
        if n>maximum:raise InvalidPreset('excessive record count')
        return n
    def blob(self):return self.take(self.integer())
    def string(self):
        raw=self.blob()
        if len(raw)%2:raise InvalidPreset('odd UTF-16 length')
        try:return raw.decode('utf-16-be')
        except UnicodeError as error:raise InvalidPreset('invalid UTF-16') from error
    def finish(self):
        if self.pos!=len(self.data):raise InvalidPreset('unexpected trailing data')

def read_xml(path,root):
    data=Path(path).read_bytes()
    if len(data)>LIMIT:raise InvalidPreset('oversized XML')
    if b'<!DOCTYPE' in data.upper() or b'<!ENTITY' in data.upper():raise InvalidPreset('XML declarations not permitted')
    tree=ET.fromstring(data)
    if tree.tag!=root:raise InvalidPreset('unexpected XML root')
    return tree

def parse_inner(data):
    r=Reader(data)
    if r.integer()!=1:raise InvalidPreset('unknown inner version')
    records={}
    for _ in range(r.count(100000)):
        label=r.string()
        if label in records:raise InvalidPreset('duplicate command label')
        sequences=[]
        for _ in range(r.count(64)):
            # Observed Qt serialization stores one key or a four-slot sequence.
            count=r.integer()
            if count not in (1,4):raise InvalidPreset('unknown sequence encoding')
            sequence=tuple(r.integer() for _ in range(count))
            keys=tuple(key for key in sequence if key)
            if sequence[:len(keys)]!=keys:raise InvalidPreset('sparse sequence')
            if keys:sequences.append(keys)
        records[label]=sequences
    base=r.string() if r.pos<len(r.data) else None;r.finish()
    return records,base

def parse_outer(data):
    r=Reader(data)
    if r.integer()!=1:raise InvalidPreset('unknown outer version')
    presets={}
    for _ in range(r.count(256)):
        name=r.string()
        if name in presets:raise InvalidPreset('duplicate preset name')
        presets[name]=parse_inner(r.blob())
    r.finish();return presets

def audit(preset_path,config_path):
    xml=read_xml(preset_path,'SmKeyboardPresetList');text=xml.findtext('PresetListBA') or ''
    if len(text)>LIMIT*2:raise InvalidPreset('oversized hex')
    try:presets=parse_outer(bytes.fromhex(text))
    except ValueError as error:raise InvalidPreset(str(error)) from error
    config=ET.parse(config_path).getroot()
    active=config.findtext('.//CustomKeyboardPreset')
    records,base=presets.get(active,({},None))
    owners=collections.defaultdict(list)
    for label,sequences in records.items():
        for sequence in sequences:owners[sequence].append(label)
    selected={k:v for k,v in records.items() if k.startswith('OpenGate - ')}
    collisions={k:[owners[s] for s in sequences if len(owners[s])>1] for k,sequences in selected.items()}
    return dict(active_preset=active,active_present=active in presets,preset_count=len(presets),base=base,
        opengate_commands=len(selected),assigned={k:v for k,v in selected.items() if v},
        collisions={k:v for k,v in collisions.items() if v},
        limitation='Persisted snapshot only; active in-memory preset and save timing unverified.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--directory',type=Path,default=Path.home()/'Library/Preferences/Blackmagic Design/DaVinci Resolve');args=p.parse_args()
    print(json.dumps(audit(args.directory/'keyboard.preset.xml',args.directory/'config.user.xml'),indent=2))
