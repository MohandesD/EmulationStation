# Editable configuration and themes

No binary rebuild is needed to change games, paths, metadata or the theme.
The starter `es_systems.cfg` describes all available selected profiles in the
stock Salvia 1.0.10 Xbox archive, including empty systems. ES hides empty lists.
The catalog is not limited to the six tested games or the original user's library.
It does not promise that every listed emulator works correctly on every game.

- App: `Hdd1:/Apps/EmulationStation` (fixed for this preview's return route).
- Maintenance dashboard: `Hdd1:/Apps/Aurora/Aurora.xex` (fixed).
- Default emulator root: `hdd:/Emulators/Salvia` (editable in systems config).
- Default ROM root: `hdd:/ROMs` (editable; keep Salvia roms_path consistent).
- ES settings: `.emulationstation/es_settings.cfg`, sibling XML elements with
  no wrapping root. Do not create duplicate setting names.
- Systems/routes: `.emulationstation/es_systems.cfg`, a `<systemList>` document.
- Metadata: `.emulationstation/gamelists/<system>/gamelist.xml`, optional.
- Themes: `.emulationstation/themes/<theme>/<system>/theme.xml`, format 3.
  Select the folder name in `ThemeSet` or the app's UI settings. Keep theme
  assets/licenses together; compatibility with newer ES-DE theme formats is not
  implied. The included Clean theme and its source are MIT-licensed original XML.

For a different local Salvia inventory, the optional helper needs Python 3.11+
on the PC, but it is not required for the default layout or on the Xbox:

```text
py -3 tools/configure.py --salvia "C:\My Salvia copy" --output "C:\New ES config"
```

It reads configs and XEX headers from that local directory, generates a catalog
and `es_systems.cfg`, and writes a **proposal** `salvia.cfg.proposed` to a new
output directory. It does not contact a console, edit its input, copy ROMs, enable
games or alter startup/security settings. The output directory must not exist.
Optional `--app hdd:/Emulators/Salvia --roms hdd:/ROMs` sets the Xbox paths.
Only `hdd:/` paths are accepted by this newcomer helper. Inspect the result,
then manually copy only the ES systems file after backing up your current one.

The proposal can append missing Salvia contexts while retaining their existing
order. Compare it with your current salvia.cfg; do not install it wholesale from
an old snapshot. Missing selected cores are listed as unavailable rather than
silently replaced with another emulator. Full `.xex` binaries are needed locally
to check headers, but are neither copied nor redistributed by this helper.

An optional `py -3 tools/verify.py` verifies every shipped file against the
manifest. Run it immediately after extraction, before making intentional edits.
It reports modified or missing files and rejects path traversal and extra files.
The archive also contains ordinary SHA-256 checksums for use with other tools.
