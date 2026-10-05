"""Capture an exclusive savestate slot, or inspect a state using local 7-Zip."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import time
import zipfile
from pine import Client

def sevenzip(explicit):
    result=explicit or shutil.which('7z')
    if not result:raise RuntimeError('Pass --sevenzip with a local 7-Zip executable')
    return result

def verify(path, executable):
    with zipfile.ZipFile(path) as archive:
        names=archive.namelist()
        if sum(info.file_size for info in archive.infolist())>128*1024*1024:raise RuntimeError('State exceeds the diagnostic size bound')
        if 'PCSX2 Internal Structures.dat' in names and archive.getinfo('PCSX2 Internal Structures.dat').file_size>8*1024*1024:raise RuntimeError('Internal metadata exceeds bound')
        if not {'eeMemory.bin','Scratchpad.bin','PCSX2 Internal Structures.dat'}<=set(names):raise RuntimeError('Required state components missing')
    subprocess.run([executable,'t',str(path)],check=True,capture_output=True)
    return names

def member(path, name, executable):
    result=subprocess.run([executable,'e','-so',str(path),name],check=True,capture_output=True)
    if len(result.stdout)>8*1024*1024:raise RuntimeError('Metadata component exceeds bound')
    return result.stdout

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sevenzip')
    sub=p.add_subparsers(dest='command',required=True)
    c=sub.add_parser('capture');c.add_argument('--port',type=int,required=True)
    c.add_argument('--directory',type=Path,required=True);c.add_argument('--slot',type=int,required=True)
    i=sub.add_parser('inspect');i.add_argument('--state',type=Path,required=True)
    i.add_argument('--output',type=Path,required=True)
    args=p.parse_args(); executable=sevenzip(args.sevenzip)
    if args.command=='capture':
        if not 0<=args.slot<=255 or not args.directory.is_dir():p.error('Existing state directory and slot 0–255 required')
        matches=lambda:list(args.directory.glob('*.'+str(args.slot)+'.p2s'))
        if matches():raise RuntimeError('Slot already exists; choose an unused slot')
        with Client(args.port) as client:client.save(args.slot)
        deadline=time.monotonic()+30
        while time.monotonic()<deadline:
            paths=matches()
            if len(paths)>1:raise RuntimeError('Ambiguous state files')
            if paths:
                try:verify(paths[0],executable);print(str(paths[0].resolve()));return
                except (zipfile.BadZipFile,subprocess.CalledProcessError,RuntimeError):pass
            time.sleep(.2)
        raise RuntimeError('State capture did not finish')
    names=verify(args.state,executable)
    version=member(args.state,'PCSX2 Savestate Version.id',executable)
    if len(version)<4 or struct.unpack_from('<I',version)[0]!=0x9a550000:raise RuntimeError('Unsupported state version for this register decoder')
    report={'components':names,'bytes':args.state.stat().st_size,
            'register_layout':'PCSX2 v2.6.3 x64; verify against source before interpreting another version'}
    registers=member(args.state,'PCSX2 Internal Structures.dat',executable)
    tag=b'cpuRegs'+bytes(25)
    position=registers[:4096].find(tag)
    if position<0 or registers[:4096].find(tag,position+1)>=0:raise RuntimeError('Unknown or ambiguous register state tag')
    offset=position+32
    if len(registers)<offset+984:raise RuntimeError('Register state is too short')
    u32=lambda at:struct.unpack_from('<I',registers,offset+at)[0]
    report['ee']={'pc':hex(u32(680)),'code_opcode':u32(684)>>26,'cycle':u32(960),
                  'status':hex(u32(592)),'exception_cause':hex(u32(596)),
                  'epc':hex(u32(600)),'config':hex(u32(608)),
                  'dual_issue_enabled':bool(u32(608)&(1<<18)),
                  'return_address':hex(u32(31*16))}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':main()
