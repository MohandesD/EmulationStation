"""Offline-only catalog preparation; never edits Salvia or contacts a console."""
import argparse
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
from system_catalog import build_catalog, choose_profiles, parse_config, systems_xml, proposed_salvia_config


def console_path(value):
    if not re.fullmatch(r'hdd:/[A-Za-z0-9 _.-]+(?:/[A-Za-z0-9 _.-]+)*', value):
        raise ValueError('Use an absolute hdd:/ path without quotes or shell characters')
    if any(part in ('.', '..') or part.endswith((' ', '.')) for part in value[5:].split('/')):
        raise ValueError('Unsafe path component')
    return value


def configure(salvia, output, app='hdd:/Emulators/Salvia', roms='hdd:/ROMs'):
    console_path(app); console_path(roms)
    salvia = salvia.resolve(strict=True)
    output = output.resolve()
    if output.exists() or output == salvia or salvia in output.parents:
        raise ValueError('Choose a NEW output directory outside the Salvia input')
    text = (salvia / 'salvia.cfg').read_text(encoding='utf-8-sig')
    main = parse_config(text)
    contexts = main.get('emulators', '')
    if not re.fullmatch(r'[a-z0-9_-]+(?: [a-z0-9_-]+)*', contexts):
        raise ValueError('Salvia emulators list must use single spaces and valid names')
    if len(contexts.split()) != len(set(contexts.split())):
        raise ValueError('Duplicate Salvia contexts')
    configs = {p.stem: p.read_text(encoding='utf-8-sig') for p in (salvia / 'config').glob('*.cfg')}
    if not configs or any(name not in configs for name in contexts.split()):
        raise ValueError('Missing Salvia configuration/predecessor file')
    installed = []
    for p in salvia.glob('*.xex'):
        with p.open('rb') as f:
            if f.read(4) == b'XEX2' and p.stat().st_size > 4096:
                installed.append(p.name)
    profiles = build_catalog(configs, installed, contexts.split())
    for p in profiles:
        p['validation'] = 'Catalog definition only; game compatibility is not established'
    selected = choose_profiles(profiles)
    if not selected:
        raise ValueError('No selected cores have valid XEX headers')
    menu = salvia / 'menu.tmp'
    menu_ready = menu.is_file() and menu.stat().st_size == 8496
    result = dict(profiles=profiles, sections=selected, menu_state_ready=menu_ready,
                  menu_note='Start and exit Salvia on the Xbox before game handoff' if not menu_ready else 'Size only checked; runtime validates structure',
                  app=app, roms=roms, console_contacted=False,
                  input_modified=False, game_launch_enabled=False)
    output.mkdir(parents=True, exist_ok=False)
    (output/'es_systems.cfg').write_text(systems_xml(selected, app, roms), encoding='utf-8')
    (output/'salvia.cfg.proposed').write_text(proposed_salvia_config(text, selected), encoding='utf-8')
    (output/'catalog.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--salvia', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--app', default='hdd:/Emulators/Salvia')
    parser.add_argument('--roms', default='hdd:/ROMs')
    args = parser.parse_args()
    try:
        result = configure(args.salvia, args.output, args.app, args.roms)
    except (OSError, ValueError) as error:
        parser.exit(1, str(error)+'\n')
    print(json.dumps(dict(sections=len(result['sections']), profiles=len(result['profiles']),
                         menu_state_ready=result['menu_state_ready'], console_contacted=False)))
