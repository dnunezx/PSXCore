"""Sample user-described memory fields without writing guest memory."""
import argparse
import json
from pathlib import Path
import time
from pine import Client

def observe(client, fields, seconds, interval):
    addresses=[int(f['address'],0) for f in fields]
    start=time.perf_counter(); samples=[]
    while time.perf_counter()-start<seconds:
        values=client.read32(addresses)
        samples.append({'seconds':time.perf_counter()-start,'values':values})
        time.sleep(interval)
    if len(samples)<2: raise RuntimeError('Too few observations')
    elapsed=samples[-1]['seconds']-samples[0]['seconds']; summary=[]
    for i,field in enumerate(fields):
        values=[s['values'][i] for s in samples]
        changes=sum(a!=b for a,b in zip(values,values[1:]))
        item={'name':field['name'],'address':hex(addresses[i]),'kind':field['kind'],
              'first':values[0],'last':values[-1],'changes_per_second':changes/elapsed,
              'minimum':min(values),'maximum':max(values),'unique_values':len(set(values))}
        if field['kind']=='counter':
            deltas=[(b-a)&0xffffffff for a,b in zip(values,values[1:])]
            item['counter_discontinuities']=sum(d>0x7fffffff for d in deltas)
            item['increments_per_second']=None if item['counter_discontinuities'] else sum(deltas)/elapsed
        summary.append(item)
    report={'seconds':elapsed,'sample_count':len(samples),'atomic':False,
            'polling_overhead_included':True,'summary':summary,'samples':samples}
    return report

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port',type=int,required=True)
    p.add_argument('--fields',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--seconds',type=float,default=10)
    p.add_argument('--interval',type=float,default=.02)
    args=p.parse_args()
    if not 1<=args.port<=65535 or not .1<=args.seconds<=60 or not .001<=args.interval<=1:p.error('Invalid bounds')
    fields=json.loads(args.fields.read_text())['fields']
    if not 1<=len(fields)<=128 or len({f['name'] for f in fields})!=len(fields):p.error('Require unique field names')
    if any(f['kind'] not in ('counter','state') for f in fields):p.error('Kinds are counter or state')
    with Client(args.port) as client:report=observe(client,fields,args.seconds,args.interval)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='samples'},indent=2))

if __name__=='__main__':main()
