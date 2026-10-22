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

# IDs identify carousel sections/theme folders, not a list of the user's games.
ALIASES = {"gameboy": "gb", "master": "mastersystem", "megacd": "segacd",
           "32x": "sega32x", "pce": "pcengine", "pcecd": "pcenginecd",
           "virtualboy": "virtualboy", "spectrum": "zxspectrum",
           "amiga500": "amiga", "fbn": "arcade", "mame": "arcade",
           "ngp-beetle": "ngp"}
TITLES = {"nes": "Nintendo Entertainment System", "snes": "Super Nintendo",
          "gameboy": "Game Boy / Game Boy Color", "genesis": "Sega Genesis / Mega Drive",
          "fbn": "Arcade", "mame": "Arcade", "ngp": "Neo Geo Pocket",
          "ngp-beetle": "Neo Geo Pocket"}

# Scrapers use es-app/src/PlatformId.cpp's vocabulary, which is separate from
# the carousel/theme IDs above. Do not rename sections or merge their ROM paths
# merely because several profiles share one scraper platform. Unknown profiles
# still browse/launch normally; "ignore" avoids scraping an unrelated platform.
SCRAPER_PLATFORMS = {
    "32x": "sega32x", "3do": "3do",
    "amiga500": "amiga", "amiga1200": "amiga", "amigacd32": "amiga",
    "atari5200": "atari5200", "atari800": "atari800", "c64": "c64",
    "cannonball": "arcade", "dos": "pc", "fbn": "arcade", "mame": "arcade",
    "gameboy": "gb gbc", "gamegear": "gamegear", "gba": "gba",
    "genesis": "genesis", "lynx": "atarilynx", "master": "mastersystem",
    "megacd": "segacd", "msx": "msx", "n64": "n64", "nes": "nes",
    "ngp": "ngp ngpc", "ngp-beetle": "ngp ngpc",
    "pce": "pcengine", "pcecd": "pcengine",
    # Salvia's pcfx.cfg is its SuperGrafx profile, not a PC-FX emulator.
    "pcfx": "pcengine", "prboom": "pc", "psx": "psx", "quake": "pc",
    "snes": "snes", "spectrum": "zxspectrum",
    "wonderswan": "wonderswan wonderswancolor",
}

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
            scraper_platform=SCRAPER_PLATFORMS.get(name, 'ignore'),
            title=TITLES.get(name, cfg.get('name', name)), folder=folder, assets=assets,
            extensions=['.' + e.lower() for e in extensions], core=selected or cores[0],
            alternatives=cores[1:], available=ready, active=name in active,
            validation='user-confirmed NES Tetris only' if name == 'nes' else 'unverified with EmulationStation'))
    return profiles

def choose_profiles(profiles, preferred=None):
    # Several backends can share one carousel section without losing any of
    # their ROM directories or launch contexts. The preferred profile supplies
    # the section's presentation and legacy command, not every game's core.
    # Built-in preferences are presentation defaults; only a supplied choice
    # is binding. Unavailable sources stay excluded, never rerouted to a core
    # belonging to another profile just because that core is available.
    explicit = preferred or {}
    preferred = dict({'arcade': 'fbn', 'ngp': 'ngp'}, **explicit)
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
        if wanted and match is None and system in explicit:
            continue  # Don't silently change the requested backend.
        primary = match or next((p for p in available if p['active']), available[0])
        routes = [primary] + [p for p in available if p is not primary]
        section = {k: v for k, v in primary.items() if k != 'routes'}
        section['routes'] = [{k: v for k, v in p.items() if k != 'routes'} for p in routes]
        selected.append(section)
    return selected

def systems_xml(profiles, app='hdd:/Emulators/Salvia', roms='hdd:/ROMs'):
    def extensions(profiles):
        values = {ext for p in profiles for ext in p['extensions']}
        return ' '.join(sorted(values | {ext.upper() for ext in values}))

    def command(p):
        return f'salvia {p["config"]} "{app}/{p["core"]}" %ROM%'

    root = ET.Element('systemList')
    for p in profiles:
        routes = p.get('routes', [p])
        node = ET.SubElement(root, 'system')
        values = dict(name=p['system'], fullname=p['title'],
            path=roms if len(routes) > 1 else roms + '/' + p['folder'],
            extension=extensions(routes), command=command(p),
            platform=p['scraper_platform'], theme=p['system'])
        for key, value in values.items():
            ET.SubElement(node, key).text = value
        if len(routes) > 1:
            # Requires the Xbox route-aware scanner/launcher. The common root
            # is a collection namespace; only these source paths are scanned.
            collection = ET.SubElement(node, 'xboxRoutes')
            for source in routes:
                route = ET.SubElement(collection, 'route')
                for key, value in dict(path=roms + '/' + source['folder'],
                        extension=extensions([source]), command=command(source)).items():
                    ET.SubElement(route, key).text = value
    ET.indent(root)
    return ET.tostring(root, encoding='unicode') + '\n'

def proposed_salvia_config(text, profiles):
    active = parse_config(text).get('emulators', '').split()
    if len(active) != len(set(active)):
        raise ValueError('Duplicate active Salvia system')
    # Append only. Existing menu.tmp indices keep their current meaning.
    for p in profiles:
        for source in p.get('routes', [p]):
            if source['config'] not in active:
                active.append(source['config'])
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
    (args.output / 'catalog.json').write_text(json.dumps(dict(profiles=catalog, sections=selected,
        frontend_systems=len(selected), hardware_verified_all_platforms=False,
        includes_empty_systems=True, requires_xbox_routes=any(len(p['routes']) > 1 for p in selected),
        console_changed=False), indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(profiles=len(catalog), frontend_systems=len(selected), console_changed=False)))

if __name__ == '__main__':
    main()
