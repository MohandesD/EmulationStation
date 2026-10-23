"""Build a redistribution ZIP from explicit source/build inputs; no upload/deploy.

Run in Linux/WSL after building the native XEX. Output must be a new directory.
Only allowlisted runtime files, source, original theme, configs and notices enter
the package. The separately obtained Salvia archive supplies inventory facts only.
"""
import argparse
import csv
import hashlib
import io
import json
import subprocess
import sys
import zipfile
from pathlib import Path
sys.dont_write_bytecode = True
from system_catalog import build_catalog, choose_profiles, parse_config, systems_xml

VERSION = '0.1.0-preview.1'
SALVIA_SHA = '7b135e1708a5add04616ed1d797357e4e9badb249f662ff28f6d706a9944fc98'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args])


def write(root, name, data):
    path = root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data.encode('utf-8') if isinstance(data, str) else data)


def zip_files(root):
    output = io.BytesIO()
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for path in sorted(root.rglob('*')):
            if path.is_file():
                if path.is_symlink():
                    raise ValueError('Symlink in package')
                info = zipfile.ZipInfo(path.relative_to(root).as_posix(), (2026,10,22,0,0,0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                z.writestr(info, path.read_bytes())
    return output.getvalue()


def build(args):
    source, builddir, deps, sdk = args.source, args.build, args.deps, args.sdk
    dist = source / 'platform/xbox360/distribution'
    pins = json.loads((source/'platform/xbox360/dependencies.json').read_text())
    for name, root, expected in [('xsdk',sdk,pins['sdk']['commit'])] + [
            (name,deps/name,entry['commit']) for name,entry in pins['sources'].items()]:
        if git(root,'rev-parse','HEAD').decode().strip() != expected:
            raise ValueError('Wrong source revision: '+name)
        if git(root,'diff','--name-only','HEAD'):
            raise ValueError('Modified dependency source: '+name)
    commit = git(source,'rev-parse','HEAD').decode().strip()
    if git(source,'status','--porcelain','--untracked-files=normal'):
        raise ValueError('Commit reviewed source/package changes locally before packaging')
    xex = (builddir/'es-app/emulationstation.xex').read_bytes()
    if not xex.startswith(b'XEX2') or len(xex) < 1000000:
        raise ValueError('Invalid native application output')
    ninja = (builddir/'build.ninja').read_text()
    if '-DEIGEN_MPL2_ONLY' not in ninja or 'powerpc-xsdk-elf' not in (builddir/'CMakeFiles/rules.ninja').read_text():
        raise ValueError('Build must use xsdk and the Eigen MPL2 guard')
    archive = args.salvia_zip.read_bytes()
    if sha(archive) != SALVIA_SHA:
        raise ValueError('Salvia inventory does not match upstream release digest')
    with zipfile.ZipFile(io.BytesIO(archive)) as z:
        names = z.namelist()
        configs = {Path(n).stem:z.read(n).decode('utf-8-sig') for n in names
                   if n.startswith('config/') and n.endswith('.cfg')}
        main = parse_config(z.read('salvia.cfg').decode('utf-8-sig'))
        installed = [n for n in names if '/' not in n and n.lower().endswith('.xex')
                     and z.getinfo(n).file_size > 4096 and z.open(n).read(4) == b'XEX2']
    profiles = build_catalog(configs, installed, main['emulators'].split())
    for profile in profiles:
        profile['validation'] = 'Definition from stock Salvia 1.0.10; not universal game compatibility'
    sections = choose_profiles(profiles)
    args.output.mkdir(parents=True, exist_ok=False)
    root = args.output / ('EmulationStation-Xbox360-'+VERSION)
    app = root/'Apps/EmulationStation'
    cfg = app/'.emulationstation'
    write(app, 'emulationstation.xex', xex)
    write(app, 'cacert.pem', (builddir/'cacert.pem').read_bytes())
    for name in ['START-HERE.md','SALVIA-SETUP.md','CONFIGURATION.md','VALIDATION.md']:
        write(root,name,(dist/name).read_bytes())
    write(cfg,'es_settings.cfg',(dist/'es_settings.cfg').read_bytes())
    write(cfg,'es_systems.cfg', systems_xml(sections))
    write(cfg,'es_theme_default.xml',(dist/'clean.xml').read_bytes())
    write(cfg,'themes/clean/clean.xml',(dist/'clean.xml').read_bytes())
    write(cfg,'themes/clean/LICENSE.txt',(source/'LICENSE.md').read_bytes())
    write(cfg,'themes/clean/README.txt','Original Clean theme, copyright 2026 EmulationStation Xbox port contributors. MIT license.\nOnly original XML and embedded Open Sans fonts; no game artwork or console logos.\n')
    for section in sections:
        write(cfg,'themes/clean/'+section['system']+'/theme.xml',
              '<theme><formatVersion>3</formatVersion><include>./../clean.xml</include></theme>\n')
    for name in ['configure.py','verify.py']:
        write(root,'tools/'+name,(dist/name).read_bytes())
    write(root,'tools/system_catalog.py',(source/'platform/xbox360/system_catalog.py').read_bytes())
    write(root,'catalog.json',json.dumps(dict(profiles=profiles,sections=sections,
          source_salvia_tag='v1.0.10',source_salvia_zip_sha256=SALVIA_SHA,
          all_platforms_hardware_verified=False),indent=2)+'\n')
    rows = io.StringIO(newline='')
    writer = csv.writer(rows)
    writer.writerow(['section','salvia_context','rom_folder_under_Hdd1_ROMs','selected_core','available'])
    for p in profiles:
        writer.writerow([p['system'],p['config'],p['folder'],p['core'],p['available']])
    write(root,'ROM-FOLDERS.csv',rows.getvalue())
    # Full notices accompany the app itself, so they survive an Apps-only copy.
    notices = {
        'EmulationStation-MIT.txt':source/'LICENSE.md',
        'OpenSans-notice.txt':source/'opensans_license.txt',
        'pugixml-MIT.txt':source/'external/pugixml/pugixml_license.txt',
        'nanosvg-zlib.txt':source/'external/nanosvg/nanosvg_license.txt',
        'xsdk-MIT.txt':sdk/'LICENSE',
        'xsdk-third-party.txt':sdk/'THIRD_PARTY_NOTICES.md',
        'SDL-zlib.txt':deps/'SDL/LICENSE.txt',
        'stb-dual.txt':deps/'stb/LICENSE',
        'FreeType-FTL.txt':deps/'freetype/docs/FTL.TXT',
        'FreeType-selection.txt':deps/'freetype/LICENSE.TXT',
        'curl-license.txt':deps/'curl/COPYING',
        'MbedTLS-license.txt':deps/'mbedtls/LICENSE',
        'MPL-2.0.txt':deps/'eigen/COPYING.MPL2',
        'Eigen-BSD.txt':deps/'eigen/COPYING.BSD',
        'Eigen-MINPACK.txt':deps/'eigen/COPYING.MINPACK',
        'GCC-runtime-exception.txt':args.toolchain_work/'src/gcc-15.3.0/COPYING.RUNTIME',
        'GCC-GPL3.txt':args.toolchain_work/'src/gcc-15.3.0/COPYING3',
        'newlib-notices.txt':args.toolchain_work/'src/newlib-4.6.0.20260123/COPYING.NEWLIB',
        'Boost-1.0.txt':args.boost_license,
    }
    for name, path in notices.items():
        write(app,'licenses/'+name,path.read_bytes())
    # Retain exact auxiliary notices, including license headers in contributed C.
    auxiliary = [deps/'freetype/src/bdf/README', deps/'freetype/src/pcf/README',
        deps/'freetype/src/base/fthash.c',deps/'freetype/include/freetype/internal/fthash.h',
        deps/'freetype/src/gzip/zlib.h',deps/'freetype/src/autofit/ft-hb.c',
        deps/'mbedtls/3rdparty/everest/library/Hacl_Curve25519.c',
        deps/'mbedtls/3rdparty/everest/library/everest.c',
        deps/'mbedtls/3rdparty/p256-m/p256-m/p256-m.c',
        deps/'SDL/src/video/yuv2rgb/LICENSE',deps/'SDL/src/hidapi/LICENSE-bsd.txt']
    auxiliary += sorted((deps/'curl/LICENSES').glob('*'))
    combined = []
    for path in auxiliary:
        data = path.read_text(errors='strict')
        # Source-bearing notices keep their complete initial license comment;
        # complete license/README text is retained for non-code files.
        if path.suffix in ('.c','.h'):
            end = data.find('*/')
            if end < 0: raise ValueError('No license header: '+str(path))
            data = data[:end+2]
        combined.append('===== '+path.relative_to(deps).as_posix()+' =====\n'+data)
    write(app,'licenses/Additional-notices.txt','\n\n'.join(combined)+'\n')
    license_review = (dist/'LICENSE-REVIEW.md').read_text().replace('@SOURCE_COMMIT@',commit)
    write(app,'THIRD-PARTY-NOTICES.md',license_review)
    write(root,'LICENSE-REVIEW.md',license_review)
    # Exact MPL-covered source accompanies the package, with all upstream notices.
    write(root,'source/Eigen-source.zip',git(deps/'eigen','archive','--format=zip','HEAD',
          'Eigen','COPYING.MPL2','COPYING.BSD','COPYING.MINPACK','COPYING.LGPL','COPYING.GPL','COPYING.README'))
    write(root,'source/EmulationStation-source.zip',git(source,'archive','--format=zip','HEAD'))
    write(root,'source/SOURCES.json',json.dumps(dict(emulationstation=commit,**pins),indent=2)+'\n')
    # Reject common private/console state and check FATX names in the copy tree.
    forbidden = {'launch.ini','menu.tmp','selection.cfg','es_log.txt','original.state','active.ini'}
    files=[]
    for p in sorted(root.rglob('*')):
        if not p.is_file(): continue
        if p.name.lower() in forbidden or 'SessionReturn' in p.parts or '__pycache__' in p.parts:
            raise ValueError('Private/runtime state in package')
        if app in p.parents and any(len(part)>42 for part in p.relative_to(app).parts):
            raise ValueError('FATX component too long: '+str(p))
        data=p.read_bytes()
        files.append(dict(path=p.relative_to(root).as_posix(),size=len(data),sha256=sha(data)))
    manifest=dict(version=VERSION,source_commit=commit,xex_sha256=sha(xex),
        native_build_guard='EIGEN_MPL2_ONLY',files=files,
        salvia_included=False,roms_bios_included=False,console_contacted=False)
    write(root,'manifest.json',json.dumps(manifest,indent=2)+'\n')
    sums=''.join(f'{f["sha256"]}  {f["path"]}\n' for f in files)
    sums+=sha((root/'manifest.json').read_bytes())+'  manifest.json\n'
    write(root,'SHA256SUMS.txt',sums)
    zip_path=args.output/(root.name+'.zip')
    zip_path.write_bytes(zip_files(root))
    write(args.output,'SHA256SUMS.txt',sha(zip_path.read_bytes())+'  '+zip_path.name+'\n')
    print(json.dumps(dict(archive=str(zip_path),sha256=sha(zip_path.read_bytes()),
        xex_sha256=sha(xex),files=len(files),sections=len(sections),profiles=len(profiles))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['source','build','deps','sdk','toolchain-work','salvia-zip','boost-license','output']:
        parser.add_argument('--'+name,required=True,type=Path)
    build(parser.parse_args())
