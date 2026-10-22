import sys
import re
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from system_catalog import build_catalog, choose_profiles, systems_xml, proposed_salvia_config, SCRAPER_PLATFORMS

def config(name, core, folder=None, extensions='rom zip'):
    return f'name={name}\nexecutable={core}\ndirectory=\nrom_directory={folder or name}\nrom_extension={extensions}\nno_uncompress=no\n'

class CatalogTests(unittest.TestCase):
    def test_empty_and_future_systems_are_not_game_dependent(self):
        profiles = build_catalog({'nes': config('nes', 'Nestopia.xex'),
            'future': config('future', 'Future.xex')}, ['Nestopia.xex', 'Future.xex'], ['nes'])
        chosen = choose_profiles(profiles)
        root = ET.fromstring(systems_xml(chosen))
        self.assertEqual({s.findtext('name') for s in root}, {'nes', 'future'})
        future = next(s for s in root if s.findtext('name') == 'future')
        self.assertEqual(future.findtext('platform'), 'ignore')
        self.assertEqual(future.findtext('theme'), 'future')
        self.assertIn('salvia future ', future.findtext('command'))
        self.assertEqual(future.findtext('path'), 'hdd:/ROMs/future')
        self.assertIsNone(future.find('xboxRoutes'))
        self.assertIn('.ROM', root.find('system/extension').text)
        proposal = proposed_salvia_config('emulators=nes\nmusic=no\n', chosen)
        self.assertIn('emulators=nes future', proposal)
        self.assertIn('music=no', proposal)

    def test_one_arcade_and_explicit_alternative(self):
        profiles = build_catalog({n: config(n, n + '.xex') for n in ['fbn', 'mame']}, ['fbn.xex', 'mame.xex'])
        self.assertEqual([p['config'] for p in choose_profiles(profiles)], ['fbn'])
        chosen = choose_profiles(profiles, {'arcade': 'mame'})
        self.assertEqual([p['config'] for p in chosen], ['mame'])
        self.assertEqual([r['config'] for r in chosen[0]['routes']], ['mame', 'fbn'])
        self.assertTrue(all('routes' not in r for r in chosen[0]['routes']))
        self.assertTrue(all('routes' not in p for p in profiles))

    def test_arcade_retains_each_backends_directory_formats_and_context(self):
        profiles = build_catalog({
            'fbn': config('fbn', 'fbneo.xex', 'fbneo', 'zip 7z'),
            'mame': config('mame', 'mame-2003-plus.xex', 'mame2003/roms', 'zip'),
        }, ['fbneo.xex', 'mame-2003-plus.xex'])
        chosen = choose_profiles(profiles)
        systems = ET.fromstring(systems_xml(chosen))
        self.assertEqual(len(systems), 1)
        section = systems[0]
        self.assertEqual(section.findtext('name'), 'arcade')
        self.assertEqual(section.findtext('path'), 'hdd:/ROMs')
        self.assertEqual(set(section.findtext('extension').split()), {'.zip', '.ZIP', '.7z', '.7Z'})
        self.assertIn('salvia fbn ', section.findtext('command'))
        routes = section.findall('xboxRoutes/route')
        self.assertEqual(len(routes), 2)
        self.assertEqual(routes[0].findtext('path'), 'hdd:/ROMs/fbneo')
        self.assertEqual(routes[0].findtext('command'), 'salvia fbn "hdd:/Emulators/Salvia/fbneo.xex" %ROM%')
        self.assertEqual(routes[1].findtext('path'), 'hdd:/ROMs/mame2003/roms')
        self.assertEqual(set(routes[1].findtext('extension').split()), {'.zip', '.ZIP'})
        self.assertEqual(routes[1].findtext('command'), 'salvia mame "hdd:/Emulators/Salvia/mame-2003-plus.xex" %ROM%')
        proposal = proposed_salvia_config('emulators=nes snes fbn\nmusic=no\n', chosen)
        self.assertEqual(proposal, 'emulators=nes snes fbn mame\nmusic=no\n')
        self.assertEqual(proposed_salvia_config(proposal, chosen), proposal)

    def test_ngp_preserves_raw_rom_backend_in_one_section(self):
        profiles = build_catalog({
            'ngp': config('ngp', 'fbneo.xex', 'ngp', 'zip'),
            'ngp-beetle': config('ngp-beetle', 'beetle-ngp.xex', 'ngp-beetle', 'zip ngp ngc ngpc npc'),
        }, ['fbneo.xex', 'beetle-ngp.xex'])
        chosen = choose_profiles(profiles)
        systems = ET.fromstring(systems_xml(chosen, roms='usb:/ROMs'))
        self.assertEqual(len(systems), 1)
        section = systems[0]
        self.assertEqual(section.findtext('name'), 'ngp')
        self.assertEqual(section.findtext('path'), 'usb:/ROMs')
        self.assertTrue({'.ngp', '.NGP', '.ngc', '.NGC'} <= set(section.findtext('extension').split()))
        routes = section.findall('xboxRoutes/route')
        self.assertEqual([r.findtext('path') for r in routes], ['usb:/ROMs/ngp', 'usb:/ROMs/ngp-beetle'])
        self.assertIn('salvia ngp-beetle ', routes[1].findtext('command'))
        self.assertEqual(proposed_salvia_config('emulators=nes\n', chosen), 'emulators=nes ngp ngp-beetle\n')

    def test_missing_preferred_backend_is_not_silently_replaced(self):
        profiles = build_catalog({n: config(n, n + '.xex') for n in ['fbn', 'mame']}, ['mame.xex'])
        # Default presentation preferences must not hide an available source.
        # This retains MAME's own directory/core, never reroutes FBNeo games.
        default = choose_profiles(profiles)
        self.assertEqual([r['config'] for r in default[0]['routes']], ['mame'])
        self.assertEqual(default[0]['folder'], 'mame')
        self.assertEqual(default[0]['core'], 'mame.xex')
        self.assertEqual(choose_profiles(profiles, {'arcade': 'fbn'}), [])
        chosen = choose_profiles(profiles, {'arcade': 'mame'})
        self.assertEqual([r['config'] for r in chosen[0]['routes']], ['mame'])
        self.assertIsNone(ET.fromstring(systems_xml(chosen)).find('system/xboxRoutes'))
        # A supplied preference is binding even when its configuration is
        # absent, rather than merely present with a missing executable.
        self.assertEqual(choose_profiles([profiles[1]], {'arcade': 'fbn'}), [])
        self.assertEqual(choose_profiles([profiles[1]])[0]['config'], 'mame')

    def test_missing_selected_core_does_not_substitute(self):
        profiles = build_catalog({'snes': config('snes', 'First.xex;Second.xex')}, ['Second.xex'])
        self.assertEqual(choose_profiles(profiles), [])
        self.assertEqual(profiles[0]['alternatives'], ['Second.xex'])

    def test_invalid_config_and_traversal(self):
        for text in [config('nes', 'Bad/Path.xex'), config('nes', 'a.xex', '../escape'),
                     config('nes', 'a.xex') + 'executable=b.xex\n']:
            with self.assertRaises(ValueError):
                build_catalog({'nes': text}, ['a.xex'])

    def test_preserve_current_menu_indices(self):
        profiles = build_catalog({'gba': config('gba', 'a.xex'), 'nes': config('nes', 'a.xex')}, ['a.xex'])
        self.assertTrue(proposed_salvia_config('emulators=nes snes fbn\n', profiles).startswith('emulators=nes snes fbn gba\n'))

    def test_scraper_ids_are_supported_by_the_frontend(self):
        # Read the actual frontend enum-name table so a typo/new alias cannot
        # silently pass by being repeated in both the implementation and test.
        source = Path(__file__).resolve().parents[3] / 'es-app/src/PlatformId.cpp'
        table = source.read_text().split('const char* PlatformNames', 1)[1].split('};', 1)[0]
        supported = set(re.findall(r'"([a-z0-9]+)"', table)) - {'unknown', 'invalid'}
        for profile, ids in SCRAPER_PLATFORMS.items():
            with self.subTest(profile=profile):
                self.assertTrue(ids.split())
                self.assertTrue(set(ids.split()) <= supported)

    def test_scraper_aliases_preserve_distinct_sections_and_launch_routes(self):
        names = ['amiga500', 'amiga1200', 'amigacd32', 'lynx', 'dos', 'pcecd']
        profiles = build_catalog({n: config(n, n + '.xex') for n in names},
                                 [n + '.xex' for n in names])
        nodes = {s.findtext('name'): s for s in ET.fromstring(systems_xml(choose_profiles(profiles)))}
        expected = {'amiga': ('amiga500', 'amiga'), 'amiga1200': ('amiga1200', 'amiga'),
                    'amigacd32': ('amigacd32', 'amiga'), 'lynx': ('lynx', 'atarilynx'),
                    'dos': ('dos', 'pc'), 'pcenginecd': ('pcecd', 'pcengine')}
        self.assertEqual(set(nodes), set(expected))
        for section, (profile, scraper) in expected.items():
            with self.subTest(section=section):
                node = nodes[section]
                self.assertEqual(node.findtext('platform'), scraper)
                self.assertEqual(node.findtext('theme'), section)
                self.assertEqual(node.findtext('path'), 'hdd:/ROMs/' + profile)
                self.assertIn('salvia ' + profile + ' ', node.findtext('command'))

    def test_combined_handhelds_include_color_scraper_ids(self):
        expected = {'gameboy': 'gb gbc', 'ngp-beetle': 'ngp ngpc',
                    'wonderswan': 'wonderswan wonderswancolor'}
        profiles = build_catalog({n: config(n, 'core.xex') for n in expected}, ['core.xex'])
        for profile in profiles:
            self.assertEqual(profile['scraper_platform'], expected[profile['config']])

if __name__ == '__main__':
    unittest.main()
