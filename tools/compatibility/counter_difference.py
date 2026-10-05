"""Measure a wrapping 32-bit clock reconstructed from two observed fields."""
import argparse
import json
from pathlib import Path

def difference(report, left, right):
    names=[f['name'] for f in report['summary']]
    if len(set(names))!=len(names):raise ValueError('Ambiguous field names')
    a,b=names.index(left),names.index(right)
    samples=report['samples']
    if len(samples)<2:raise ValueError('Require at least two samples')
    values=[(s['values'][a]-s['values'][b])&0xffffffff for s in samples]
    elapsed=samples[-1]['seconds']-samples[0]['seconds']
    if elapsed<=0:raise ValueError('Require increasing observation times')
    deltas=[(y-x)&0xffffffff for x,y in zip(values,values[1:])]
    discontinuities=sum(d>0x7fffffff for d in deltas)
    result={'operation':'(left - right) modulo 2^32','left':left,'right':right,
            'seconds':elapsed,'first':values[0],'last':values[-1],
            'counter_discontinuities':discontinuities,
            'increments_per_second':None if discontinuities else sum(deltas)/elapsed,
            'limitations':'Source reads are not atomic. Validate the clock formula against the exact implementation.'}
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input',type=Path,required=True)
    p.add_argument('--left',required=True);p.add_argument('--right',required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if args.input.stat().st_size>16*1024*1024:p.error('Observation exceeds 16 MiB bound')
    result=difference(json.loads(args.input.read_text()),args.left,args.right)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
