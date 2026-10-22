import sys
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from system_catalog import build_catalog, choose_profiles, systems_xml, proposed_salvia_config

def config(name, core, folder=None):
    return f'name={name}\nexecutable={core}\ndirectory=\nrom_directory={folder or name}\nrom_extension=rom zip\nno_uncompress=no\n'

class CatalogTests(unittest.TestCase):
    def test_empty_and_future_systems_are_not_game_dependent(self):
        profiles = build_catalog({'nes': config('nes', 'Nestopia.xex'),
            'future': config('future', 'Future.xex')}, ['Nestopia.xex', 'Future.xex'], ['nes'])
        chosen = choose_profiles(profiles)
        root = ET.fromstring(systems_xml(chosen))
        self.assertEqual({s.findtext('name') for s in root}, {'nes', 'future'})
        self.assertIn('.ROM', root.find('system/extension').text)
        proposal = proposed_salvia_config('emulators=nes\nmusic=no\n', chosen)
        self.assertIn('emulators=nes future', proposal)
        self.assertIn('music=no', proposal)

    def test_one_arcade_and_explicit_alternative(self):
        profiles = build_catalog({n: config(n, n + '.xex') for n in ['fbn', 'mame']}, ['fbn.xex', 'mame.xex'])
        self.assertEqual([p['config'] for p in choose_profiles(profiles)], ['fbn'])
        self.assertEqual([p['config'] for p in choose_profiles(profiles, {'arcade': 'mame'})], ['mame'])

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

if __name__ == '__main__':
    unittest.main()
