#!/usr/bin/env python3
"""Check native Windows Desktop identity and every embedded image against the supplied ICO."""
from __future__ import annotations
import argparse,hashlib,json,pathlib,struct
from audit_pe import PE

def resources(pe):
    base,size=pe.directory(2);items={}
    if not base:raise ValueError('EXE has no resources')
    def walk(relative,path):
        off=pe.offset(base+relative,16);named,ids=struct.unpack_from('<HH',pe.data,off+12)
        for i in range(named+ids):
            key,nextoff=struct.unpack_from('<II',pe.data,off+16+8*i)
            label=key
            if key&0x80000000:
                pos=pe.offset(base+(key&0x7fffffff),2);n=pe.u16(pos)
                label=pe.data[pos+2:pos+2+2*n].decode('utf-16-le')
            if nextoff&0x80000000:walk(nextoff&0x7fffffff,path+(label,))
            else:
                pos=pe.offset(base+nextoff,16);rva,n=struct.unpack_from('<II',pe.data,pos)
                start=pe.offset(rva,n);items[path+(label,)]=pe.data[start:start+n]
    walk(0,());return items

def audit(exe,icon):
    pe=PE(exe);rs=resources(pe);ico=icon.read_bytes();reserved,typ,count=struct.unpack_from('<HHH',ico)
    if reserved!=0 or typ!=1:raise ValueError('Source artwork is not ICO')
    source=[]
    for i in range(count):
        w,h,c,res,planes,bits,n,off=struct.unpack_from('<BBBBHHII',ico,6+16*i)
        source.append(((w,h,c,res,max(1,planes),bits,n),ico[off:off+n]))
    groups=[v for k,v in rs.items() if k[0]==14];errors=[]
    if len(groups)!=1:errors.append(f'Expected one icon group, found {len(groups)}')
    matches=0
    for g in groups:
        _,kind,n=struct.unpack_from('<HHH',g)
        if kind!=1 or n!=count:errors.append('Embedded icon count differs from original Desktop ICO');continue
        for i in range(n):
            w,h,c,res,planes,bits,size,id=struct.unpack_from('<BBBBHHIH',g,6+14*i)
            candidates=[v for k,v in rs.items() if k[0]==3 and k[1]==id]
            if (w,h,c,res,planes,bits,size)!=source[i][0] or not candidates or any(v!=source[i][1] for v in candidates):errors.append(f'Icon image {i} differs from Desktop artwork')
            else:matches+=1
    versions=[v for k,v in rs.items() if k[0]==16];fileversion=productversion=None
    for block in versions:
        off=block.find(struct.pack('<I',0xfeef04bd))
        if off<0:continue
        values=struct.unpack_from('<13I',block,off)
        fileversion='.'.join(map(str,[values[2]>>16,values[2]&65535,values[3]>>16,values[3]&65535]))
        productversion='.'.join(map(str,[values[4]>>16,values[4]&65535,values[5]>>16,values[5]&65535]))
    if fileversion!='1.0.0.0' or productversion!='1.0.0.0':errors.append('Native product/file version must be 1.0.0.0')
    header=pe.u32(0x3c);subsystem=pe.u16(header+24+68)
    if pe.machine!=0x8664 or subsystem!=2:errors.append('Expected native x64 Windows GUI executable')
    return {'version':'1.0.0','edition':'Desktop','machine':hex(pe.machine),'subsystem':subsystem,'fileVersion':fileversion,'productVersion':productversion,'desktopIconSHA256':hashlib.sha256(ico).hexdigest(),'sourceIconImages':count,'matchingEmbeddedIconImages':matches,'iconGroups':len(groups),'errors':errors,'passed':not errors}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('exe',type=pathlib.Path);p.add_argument('icon',type=pathlib.Path);p.add_argument('--json',type=pathlib.Path,required=True);a=p.parse_args()
    r=audit(a.exe,a.icon);a.json.parent.mkdir(parents=True,exist_ok=True);a.json.write_text(json.dumps(r,indent=2)+'\n');print(f"Windows resources: {'PASS' if r['passed'] else 'FAIL'}; {r['matchingEmbeddedIconImages']} Desktop icon images; file version {r['fileVersion']}");return 0 if r['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
