#!/usr/bin/env python3
"""Fetch pinned source dependencies without overwriting an existing checkout.

Pass --boost-include /usr/include after installing Boost 1.83 development
headers. Only boost/ is copied; never add host libc headers to a cross build.
This prepares source trees only. It does not compile, install or deploy a title.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess

def git(path, *args):
    return subprocess.check_output(['git', '-C', str(path), *args], text=True).strip()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination',type=Path)
    parser.add_argument('--boost-include',type=Path,required=True)
    args=parser.parse_args()
    lock=json.loads(Path(__file__).with_name('dependencies.json').read_text())
    root=args.destination.resolve()
    root.mkdir(parents=True,exist_ok=True)
    for name,entry in lock['sources'].items():
        path=root/name
        if not path.exists():
            subprocess.run(['git','clone','--filter=blob:none','--no-checkout',entry['url'],str(path)],check=True)
            git(path,'checkout','--detach',entry['commit'])
        if git(path,'rev-parse','HEAD')!=entry['commit']:
            raise SystemExit(f'{path}: wrong revision; refusing to reset an existing checkout')
        if git(path,'status','--porcelain','--untracked-files=no'):
            raise SystemExit(f'{path}: tracked local changes; refusing to use modified dependencies')
        if entry.get('submodules'):
            git(path,'submodule','update','--init','--recursive')
        print(name,entry['commit'],flush=True)
    source=args.boost_include.resolve()/'boost'
    version=source/'version.hpp'
    match=re.search(r'^#define BOOST_VERSION\s+(\d+)\s*$',version.read_text(),re.M)
    if not match or int(match[1])!=lock['boost_headers']['BOOST_VERSION']:
        raise SystemExit('Expected Boost 1.83 headers')
    destination=root/'boost'/'boost'
    if not destination.exists():
        shutil.copytree(source,destination)
    elif (destination/'version.hpp').read_bytes()!=version.read_bytes():
        raise SystemExit('Existing isolated Boost headers differ; refusing to replace them')
    print('Prepared source dependencies; SDK/toolchain build is a separate step.')

if __name__=='__main__':
    main()
