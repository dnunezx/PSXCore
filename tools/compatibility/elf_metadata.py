"""Inspect bounded MIPS ELF32 control-flow and memory-reference metadata."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

def inspect(data, expected_hash, start, end, fields=()):
    digest=hashlib.sha256(data).hexdigest()
    if digest!=expected_hash.lower():raise ValueError('ELF fingerprint mismatch')
    if data[:7]!=b'\x7fELF\x01\x01\x01' or len(data)<52:raise ValueError('Require little-endian ELF32')
    if struct.unpack_from('<H',data,18)[0]!=8:raise ValueError('Require MIPS architecture')
    table=struct.unpack_from('<I',data,28)[0]
    stride,count=struct.unpack_from('<HH',data,42)
    if stride!=32 or not 1<=count<=64 or table+stride*count>len(data):raise ValueError('Invalid program table')
    segments=[]
    for i in range(count):
        kind,offset,address,physical,size,memory,flags,alignment=struct.unpack_from('<8I',data,table+i*stride)
        if kind==1:
            if offset+size>len(data) or size>memory:raise ValueError('Invalid segment bounds')
            segments.append({'offset':offset,'address':address,'file_bytes':size,'memory_bytes':memory,'flags':flags})
    if start%4 or end%4 or not 0<end-start<=65536:raise ValueError('Require an aligned range of at most 64 KiB')
    events=[];calls=[];counts={};loops=[]
    for segment in segments:
        if not segment['flags']&1:continue
        for address in range(segment['address'],segment['address']+segment['file_bytes']-3,4):
            word=struct.unpack_from('<I',data,segment['offset']+address-segment['address'])[0]
            op=word>>26;rs=(word>>21)&31;rt=(word>>16)&31;imm=word&65535
            if imm>=32768:imm-=65536
            target=((address+4)&0xf0000000)|((word&0x3ffffff)*4)
            if op in (8,9) and rs==rt and rs!=0 and imm<0 and address+12<=segment['address']+segment['file_bytes']:
                branch,delay=struct.unpack_from('<2I',data,segment['offset']+address-segment['address']+4)
                if branch>>26==5 and ((branch>>21)&31)==rs and ((branch>>16)&31)==0 and (branch&65535)==0xfffe and delay==0:
                    loops.append({'site':hex(address),'counter_register':rs,'decrement':-imm,
                                  'pattern':'three-instruction countdown; execution unverified'})
            if op in (2,3) and start<=target<end:
                calls.append({'site':hex(address),'kind':'call' if op==3 else 'jump','target':hex(target)})
            if not start<=address<end:continue
            counts[str(op)]=counts.get(str(op),0)+1
            item={'site':hex(address)}
            if op in (2,3):item.update(kind='call' if op==3 else 'jump',target=hex(target))
            elif op in (4,5,6,7,20,21):item.update(kind='branch',opcode=op,target=hex(address+4+imm*4),left=rs,right=rt)
            elif op in (0x20,0x21,0x23,0x24,0x25,0x28,0x29,0x2b,0x37,0x3f,0x1e,0x1f):
                if fields and (imm&65535) not in fields:continue
                item.update(kind='memory',opcode=op,base_register=rs,value_register=rt,offset=imm)
            elif op==16 and rs in (0,4):item.update(kind='cop0-read' if rs==0 else 'cop0-write',register=(word>>11)&31)
            elif op==9 and rs==rt==29 and imm<0:item.update(kind='stack-frame',bytes=-imm)
            else:continue
            events.append(item)
    return {'sha256':digest,'range':[hex(start),hex(end)],'segments':segments,
            'primary_opcode_counts':counts,'events':events,'direct_references_into_range':calls,
            'static_countdown_loops':loops,
            'limitations':'Static metadata only. No instruction bytes exported; indirect calls and runtime-generated code are not resolved.'}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--elf',type=Path,required=True);p.add_argument('--sha256',required=True)
    p.add_argument('--start',type=lambda s:int(s,0),required=True);p.add_argument('--end',type=lambda s:int(s,0),required=True)
    p.add_argument('--field-offset',type=lambda s:int(s,0),action='append',default=[])
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if args.elf.stat().st_size>64*1024*1024:p.error('ELF exceeds bound')
    report=inspect(args.elf.read_bytes(),args.sha256,args.start,args.end,args.field_offset)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'events':len(report['events']),'references_into_range':report['direct_references_into_range']},indent=2))

if __name__=='__main__':main()
