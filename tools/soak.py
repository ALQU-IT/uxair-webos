#!/usr/bin/env python3
"""Read-only RAM/session sampler for a running TV app. Does not launch or stop it."""
import argparse
import datetime
import json
import os
import pathlib
import re
import subprocess
import time

ROOT=pathlib.Path(__file__).resolve().parent.parent

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--seconds',type=int,default=1800)
    parser.add_argument('--interval',type=int,default=30)
    parser.add_argument('--label',default='session')
    args=parser.parse_args()
    if args.interval<1 or args.seconds<1:parser.error('positive duration and interval required')
    out=ROOT/'build/logs'/f'soak-{args.label}.jsonl';out.parent.mkdir(exist_ok=True)
    host=os.environ.get('TV_HOST')
    key=os.environ.get('TV_KEY')
    if not host or not key:
        parser.error('TV_HOST and TV_KEY must be set (for example: TV_HOST=tv.local TV_KEY=/path/to/key)')
    key=os.path.expanduser(key)
    app=json.loads((ROOT/'packaging/appinfo.json').read_text())
    app_dir=f'/media/developer/apps/usr/palm/applications/{app["id"]}'
    log_path=f'/var/palm/jail/{app["id"]}{app_dir}/uxair.log'
    command=f"p=$(pidof uxair); if [ -n \"$p\" ]; then cat /proc/$p/status; fi; grep '\\[health\\]' {log_path} | tail -n 1"
    start=time.monotonic();deadline=start+args.seconds
    with out.open('w') as f:
        while True:
            sample={'time':datetime.datetime.now(datetime.timezone.utc).isoformat(),'elapsed':round(time.monotonic()-start,2)}
            try:
                raw=subprocess.check_output(['ssh','-tt','-i',key,'-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=8','-o','LogLevel=ERROR','root@'+host,command],text=True,timeout=15)
                for field in ['Pid','VmRSS','VmHWM','VmSize','Threads']:
                    match=re.search(r'^'+field+r':\s*(\d+)',raw,re.M)
                    if match:sample[field]=int(match[1])
                match=re.search(r'\[stats\].*',raw)
                if match:sample['stats']=match[0].strip()
                sample['running']='Pid' in sample
            except (subprocess.SubprocessError,OSError) as error:
                sample['error']=str(error)
            f.write(json.dumps(sample)+'\n');f.flush()
            if time.monotonic()>=deadline:break
            time.sleep(min(args.interval,max(0,deadline-time.monotonic())))
    print(out)

if __name__=='__main__':main()
