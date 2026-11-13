#!/usr/bin/env python3
"""Inspect ESP32 partition binary without modifying any device."""
import argparse, json, struct
from pathlib import Path

SUBTYPES={0:'factory',16:'ota_0',17:'ota_1'}
def parse_partitions(data):
    out=[]
    for i in range(0,len(data),32):
        chunk=data[i:i+32]
        if len(chunk)<32: break
        if chunk[:2]==b'\xff\xff':break
        if chunk[:2]!=b'\xaa\x50':
            if chunk[:2]==b'\xeb\xeb':continue
            raise ValueError(f'Unexpected partition entry at {i:#x}')
        _,typ,sub,off,size,name,flags=struct.unpack('<HBBII16sI',chunk)
        out.append(dict(name=name.split(b'\0')[0].decode('ascii'),type=typ,subtype=sub,offset=off,size=size,flags=flags))
    if not out:raise ValueError('No partition entries')
    ranges=sorted(out,key=lambda x:x['offset'])
    for a,b in zip(ranges,ranges[1:]):
        if a['offset']+a['size']>b['offset']:raise ValueError('Overlapping partitions')
    return out

def inspect(build):
    parts=parse_partitions((build/'partitions.bin').read_bytes())
    apps=[p for p in parts if p['type']==0]
    nvs=[p for p in parts if p['name']=='nvs']
    if not apps or not nvs:raise ValueError('Missing app or NVS partition')
    if len([p for p in apps if p['subtype']==0])==0 and len([p for p in apps if p['subtype']==16])==0:raise ValueError('No known initial app partition')
    target=next(p for p in apps if p['subtype'] in (0,16))
    firmware=build/'firmware.bin'
    if not firmware.is_file():raise ValueError('Missing firmware.bin')
    if firmware.stat().st_size>target['size']:raise ValueError('Firmware exceeds target app partition')
    if nvs[0]['offset']!=0x9000:raise ValueError('Unexpected NVS offset, manual review required')
    return {'partitions':parts,'initial_app':target,'firmware_size':firmware.stat().st_size,'nvs':nvs[0]}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('build_dir',type=Path);a=p.parse_args()
    print(json.dumps(inspect(a.build_dir),indent=2))
