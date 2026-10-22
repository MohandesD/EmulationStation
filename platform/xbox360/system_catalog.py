"""Offline platform catalog from a local Salvia configuration/core inventory.

No network, console writes, ROM reads or launches. Empty systems are included in
es_systems.cfg; EmulationStation hides empty/missing game directories at runtime.
The generated Salvia file is a proposal, never an in-place configuration edit.
"""
from pathlib import Path
import argparse
import json
import re
import xml.etree.ElementTree as ET

# IDs are presentation/scraper aliases, not a list of the user's games.
ALIASES = {"gameboy": "gb", "master": "mastersystem", "megacd": "segacd",
           "32x": "sega32x", "pce": "pcengine", "pcecd": "pcenginecd",
           "virtualboy": "virtualboy", "spectrum": "zxspectrum",
           "amiga500": "amiga", "fbn": "arcade", "mame": "arcade",
           "ngp-beetle": "ngp"}
TITLES = {"nes": "Nintendo Entertainment System", "snes": "Super Nintendo",
          "gameboy": "Game Boy / Game Boy Color", "genesis": "Sega Genesis / Mega Drive",
          "fbn": "Arcade", "mame": "Arcade", "ngp": "Neo Geo Pocket",
          "ngp-beetle": "Neo Geo Pocket"}

def parse_config(text):
    result = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith('#') or '=' not in line:
            continue
        key, value = (part.strip() for part in line.split('=', 1))
        if key in result:
            raise ValueError('Duplicate Salvia setting: ' + key)
        result[key] = value
    return result

def relative(value):
    value = value.replace('\\', '/')
    parts = value.split('/')
    if (not value or any(p in ('', '.', '..') for p in parts)
            or re.search(r'[:\x00-\x1f"<>|]', value)):
        raise ValueError('Expected a relative Salvia path')
    return value

def build_catalog(configs, installed, active=()):
    installed = {name.lower(): name for name in installed}
    profiles = []
    for name, text in sorted(configs.items()):
        if not re.fullmatch(r'[a-z0-9_-]{1,38}', name):
            raise ValueError('Invalid system configuration name')
        cfg = parse_config(text)
        folder = relative(cfg.get('rom_directory', ''))
        assets = relative(cfg.get('assets', 'assets/' + name))
        cores = [c.strip() for c in cfg.get('executable', '').split(';') if c.strip()]
        if not cores or any(not re.fullmatch(r'[A-Za-z0-9_. -]{1,38}\.xex', c, re.I) for c in cores):
            raise ValueError('Invalid core selection for ' + name)
        extensions = cfg.get('rom_extension', '').split()
        if not extensions or any(not re.fullmatch(r'[A-Za-z0-9]+', ext) for ext in extensions):
            raise ValueError('Invalid extensions for ' + name)
        # Preserve the selected (first) core. Never silently switch cores merely
        # because a different executable is available in the local inventory.
        selected = installed.get(cores[0].lower())
        ready = bool(selected and not cfg.get('directory') and cfg.get('no_uncompress') in ('yes', 'no'))
        profiles.append(dict(config=name, system=ALIASES.get(name, name),
            title=TITLES.get(name, cfg.get('name', name)), folder=folder, assets=assets,
            extensions=['.' + e.lower() for e in extensions], core=selected or cores[0],
            alternatives=cores[1:], available=ready, active=name in active,
            validation='user-confirmed NES Tetris only' if name == 'nes' else 'unverified with EmulationStation'))
    return profiles

def choose_profiles(profiles, preferred=None):
    # MAME/FBNeo and the two NGP cores are alternative backends, not extra
    # carousel sections. All remain in catalog.json for explicit selection.
    preferred = dict({'arcade': 'fbn', 'ngp': 'ngp'}, **(preferred or {}))
    groups = {}
    for p in profiles:
        groups.setdefault(p['system'], []).append(p)
    selected = []
    for system, group in groups.items():
        available = [p for p in group if p['available']]
        if not available:
            continue
        wanted = preferred.get(system)
        match = next((p for p in available if p['config'] == wanted), None)
        if wanted and any(p['config'] == wanted for p in group) and match is None:
            continue  # Don't silently change the requested backend.
        selected.append(match or next((p for p in available if p['active']), available[0]))
    return selected

def systems_xml(profiles, app='hdd:/Emulators/Salvia', roms='hdd:/ROMs'):
    root = ET.Element('systemList')
    for p in profiles:
        node = ET.SubElement(root, 'system')
        values = dict(name=p['system'], fullname=p['title'], path=roms + '/' + p['folder'],
            extension=' '.join(sorted(set(p['extensions'] + [e.upper() for e in p['extensions']]))),
            command=f'salvia {p["config"]} "{app}/{p["core"]}" %ROM%',
            platform=p['system'], theme=p['system'])
        for key, value in values.items():
            ET.SubElement(node, key).text = value
    ET.indent(root)
    return ET.tostring(root, encoding='unicode') + '\n'

def proposed_salvia_config(text, profiles):
    active = parse_config(text).get('emulators', '').split()
    if len(active) != len(set(active)):
        raise ValueError('Duplicate active Salvia system')
    # Append only. Existing menu.tmp indices keep their current meaning.
    for p in profiles:
        if p['config'] not in active:
            active.append(p['config'])
    if len(active) > 128:
        raise ValueError('Too many Salvia systems')
    lines = text.splitlines()
    indices = [i for i, line in enumerate(lines) if line.strip().startswith('emulators=')
               or re.match(r'^\s*emulators\s+=', line)]
    if len(indices) != 1:
        raise ValueError('Expected exactly one emulators setting')
    lines[indices[0]] = 'emulators=' + ' '.join(active)
    return '\n'.join(lines) + '\n'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--salvia', type=Path, required=True, help='Local config/core snapshot')
    parser.add_argument('--output', type=Path, required=True, help='New offline output directory')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    text = (args.salvia / 'salvia.cfg').read_text(encoding='utf-8-sig')
    configs = {p.stem: p.read_text(encoding='utf-8-sig') for p in (args.salvia / 'config').glob('*.cfg')}
    catalog = build_catalog(configs, [p.name for p in args.salvia.iterdir() if p.is_file()],
                            parse_config(text).get('emulators', '').split())
    selected = choose_profiles(catalog)
    (args.output / 'es_systems.cfg').write_text(systems_xml(selected), encoding='utf-8')
    (args.output / 'salvia.cfg.proposed').write_text(proposed_salvia_config(text, selected), encoding='utf-8')
    (args.output / 'catalog.json').write_text(json.dumps(dict(profiles=catalog,
        frontend_systems=len(selected), hardware_verified_all_platforms=False,
        includes_empty_systems=True, console_changed=False), indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(profiles=len(catalog), frontend_systems=len(selected), console_changed=False)))

if __name__ == '__main__':
    main()
