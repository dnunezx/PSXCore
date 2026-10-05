"""Create a guarded experimental savestate copy with one EE RAM word changed."""
import argparse
import hashlib
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zipfile
from state import sevenzip, verify

def replace_word(data,address,expected,replacement):
    if address<0 or address%4 or address+4>len(data):raise ValueError('Aligned in-range RAM address required')
    if any(not 0<=word<=0xffffffff for word in (expected,replacement)):raise ValueError('32-bit words required')
    if expected==replacement:raise ValueError('Replacement must differ')
    if struct.unpack_from('<I',data,address)[0]!=expected:raise ValueError('Expected word mismatch')
    result=bytearray(data);struct.pack_into('<I',result,address,replacement)
    return bytes(result)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--state',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sha256',required=True);parser.add_argument('--sevenzip')
    for name in ('address','expected','replacement'):parser.add_argument('--'+name,type=lambda s:int(s,0),required=True)
    args=parser.parse_args();executable=sevenzip(args.sevenzip)
    if args.output.exists():parser.error('Output exists; choose a new filename')
    if args.state.stat().st_size>128*1024*1024:parser.error('State exceeds bound')
    if hashlib.sha256(args.state.read_bytes()).hexdigest()!=args.sha256.lower():parser.error('State fingerprint mismatch')
    verify(args.state,executable)
    def directory(path):
        with zipfile.ZipFile(path) as archive:
            names=archive.namelist()
            if len(names)!=len(set(names)):raise ValueError('Duplicate archive members')
            return {i.filename:(i.file_size,i.CRC) for i in archive.infolist()}
    before=directory(args.state)
    if before['eeMemory.bin'][0]!=32*1024*1024:parser.error('Expected PCSX2 32 MiB EE RAM')
    ram=subprocess.check_output([executable,'e','-so',str(args.state.resolve()),'eeMemory.bin'])
    changed=replace_word(ram,args.address,args.expected,args.replacement)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='compat-state-') as temporary:
        member=Path(temporary)/'eeMemory.bin';member.write_bytes(changed)
        shutil.copyfile(args.state,args.output)
        subprocess.run([executable,'u','-tzip','-mm=Deflate',str(args.output.resolve()),'eeMemory.bin'],cwd=temporary,check=True,capture_output=True)
    verify(args.output,executable)
    after=directory(args.output)
    if before.keys()!=after.keys() or any(before[k]!=after[k] for k in before if k!='eeMemory.bin'):raise RuntimeError('Unrelated archive members changed')
    actual=subprocess.check_output([executable,'e','-so',str(args.output.resolve()),'eeMemory.bin'])
    if actual!=changed:raise RuntimeError('Patched RAM verification failed')
    print('Verified experimental state copy: '+str(args.output.resolve()))

if __name__=='__main__':main()
