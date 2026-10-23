"""Validate a fresh ZIP extraction plus a separate stock Salvia inventory, offline."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import subprocess
import tempfile
import zipfile


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extract(archive, dest):
    with zipfile.ZipFile(archive) as z:
        names=set()
        for info in z.infolist():
            p=PurePosixPath(info.filename)
            if p.is_absolute() or '..' in p.parts or ':' in info.filename or '\\' in info.filename:
                raise ValueError('Unsafe ZIP path')
            if info.filename.lower() in names: raise ValueError('Duplicate ZIP path')
            names.add(info.filename.lower())
        z.extractall(dest)


def run(cmd, cwd, success=True):
    env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    result=subprocess.run([str(x) for x in cmd],cwd=cwd,env=env,text=True,capture_output=True)
    if (result.returncode==0)!=success:
        raise AssertionError(result.stdout+'\n'+result.stderr)
    return result.stdout.strip()


def test(args):
    with tempfile.TemporaryDirectory(prefix='es-clean-release-') as tmp:
        tmp=Path(tmp); root=tmp/'extracted';root.mkdir()
        extract(args.archive,root)
        manifest=json.loads((root/'manifest.json').read_text())
        checks=[]
        def check(name):checks.append(name)
        run(['python3',root/'tools/verify.py'],tmp);check('full manifest and checksum verification')
        app=root/'Apps/EmulationStation'
        assert (app/'emulationstation.xex').read_bytes()[:4]==b'XEX2'
        for p in app.rglob('*'):
            assert all(len(x)<=42 for x in p.relative_to(app).parts)
        check('native XEX and FATX component lengths')
        assert len(list(root.rglob('*.xex')))==1
        for p in root.rglob('*'):
            assert p.suffix.lower() not in {'.nes','.sfc','.smc','.gb','.gbc','.z64','.n64','.rom','.iso','.joy','.dll','.pdb','.elf'}
            assert p.name.lower() not in {'menu.tmp','launch.ini','selection.cfg','original.state','es_log.txt'}
        check('no emulator binaries, games, BIOS, mapping or private runtime state')
        salvia=tmp/'salvia';salvia.mkdir();extract(args.salvia_zip,salvia)
        before={p.relative_to(salvia).as_posix():digest(p) for p in salvia.rglob('*') if p.is_file()}
        helper=root/'tools/configure.py'
        output=tmp/'configured'
        result=json.loads(run(['python3',helper,'--salvia',salvia,'--output',output],tmp))
        assert result['menu_state_ready'] is False
        assert (output/'es_systems.cfg').read_bytes()==(app/'.emulationstation/es_systems.cfg').read_bytes()
        check('catalog regenerated from pristine upstream Salvia; absent menu reported')
        run(['python3',helper,'--salvia',salvia,'--output',output],tmp,False)
        run(['python3',helper,'--salvia',salvia,'--output',salvia/'nested'],tmp,False)
        run(['python3',helper,'--salvia',salvia,'--output',tmp/'bad-path','--roms','hdd:/../ROMs'],tmp,False)
        assert not (tmp/'bad-path').exists()
        check('existing-output, input-nesting and unsafe-path rejection')
        core=salvia/'Nestopia.xex'; core.rename(salvia/'Nestopia.absent')
        missing=tmp/'missing-core'
        run(['python3',helper,'--salvia',salvia,'--output',missing],tmp)
        import xml.etree.ElementTree as ET
        assert 'nes' not in [n.findtext('name') for n in ET.parse(missing/'es_systems.cfg').getroot()]
        (salvia/'Nestopia.absent').rename(core)
        check('missing selected core omitted without silent replacement')
        predecessor=salvia/'config/genesis.cfg';predecessor.rename(salvia/'config/genesis.absent')
        run(['python3',helper,'--salvia',salvia,'--output',tmp/'missing-config'],tmp,False)
        (salvia/'config/genesis.absent').rename(predecessor)
        check('missing predecessor configuration rejected')
        after={p.relative_to(salvia).as_posix():digest(p) for p in salvia.rglob('*') if p.is_file()}
        assert before==after;check('Salvia input remains byte-identical')
        model=run([args.model,root,salvia,tmp/'model'],tmp)
        model=json.loads(model.splitlines()[-1])
        assert model['sections']==result['sections']
        check('production settings, system model, grouped routes, themes and Salvia context')
        settings=app/'.emulationstation/es_settings.cfg'
        original=settings.read_bytes();settings.write_bytes(original+b'\n')
        run(['python3',root/'tools/verify.py'],tmp,False);settings.write_bytes(original)
        check('tampered package rejected')
        run(['python3',root/'tools/verify.py'],tmp)
        check('package untouched by validation')
        report=dict(version=manifest['version'],source_commit=manifest['source_commit'],
            archive=args.archive.name,archive_sha256=digest(args.archive),xex_sha256=manifest['xex_sha256'],
            clean_extraction=True,checks_passed=checks,model=model,console_contacted=False,
            fresh_package_console_test=False,gameplay_tested_here=False,
            source_license_review='Included LICENSE-REVIEW.md and app notices',
            limitations=['No rendering/device runtime in model test','No ROM or XEX executed',
                         'New package/Clean theme not installed on working console'])
        args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report,indent=2))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ['archive','salvia-zip','model','report']:p.add_argument('--'+key,type=Path,required=True)
    args=p.parse_args()
    for key in ['archive','salvia_zip','model','report']:setattr(args,key,getattr(args,key).resolve())
    test(args)
