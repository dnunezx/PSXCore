"""Validate a bounded live code signature against one privately supplied file."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from pine import Client

def validate(code, reference):
    if not code or len(code)%4 or len(code)>512:
        raise ValueError('Require 4–512 aligned code bytes')
    location=reference.find(code)
    if location<0:raise ValueError('Live signature does not match reference')
    if reference.find(code,location+1)>=0:raise ValueError('Live signature is ambiguous')
    return {'code_sha256':hashlib.sha256(code).hexdigest(),
            'reference_sha256':hashlib.sha256(reference).hexdigest(),
            'reference_offset':location,'bytes':len(code),'unique_exact_match':True}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port',type=int,required=True)
    p.add_argument('--address',type=lambda s:int(s,0),required=True)
    p.add_argument('--bytes',type=int,default=64)
    p.add_argument('--reference',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if not 1<=args.port<=65535 or args.address%4 or not 4<=args.bytes<=512 or args.bytes%4 or not 0<=args.address<=0x100000000-args.bytes:
        p.error('Require a port and aligned 32-bit range of 4–512 bytes')
    if args.reference.stat().st_size>1024*1024*1024:p.error('Reference exceeds 1 GiB bound')
    with Client(args.port) as client:
        words=client.read32(list(range(args.address,args.address+args.bytes,4)))
    report=validate(struct.pack('<'+'I'*len(words),*words),args.reference.read_bytes())
    report['address']=hex(args.address)
    report['limitations']='Matching bytes establishes identity, not function semantics or counter meaning.'
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':main()
