# Platform catalog independent of the game collection

EmulationStation is a frontend. A system's menu entry, an available Xbox core,
and verified game compatibility are three separate states. The port is intended
to cover every platform supported by its configured Xbox emulators, not only
the games in the initial five-system installation. It does not make arbitrary
RetroPie emulators or PC cores run on Xbox 360.

`system_catalog.py` reads a **local** Salvia configuration/core inventory. It has
no FTP or XBDM operations and reads no ROM contents. Example:

```
python platform/xbox360/system_catalog.py --salvia /local/Salvia --output /new/offline-catalog
```

Outputs are a profile inventory, `es_systems.cfg`, and `salvia.cfg.proposed`.
Existing Salvia system order and other settings are retained; new contexts are
appended so saved menu indices keep their meaning. Nothing is installed or edited
in place. The proposal needs a separate reviewed deployment; merely adding an ES
entry does not enable a missing Salvia context.

All available platform definitions are included regardless of the presence of
games. ES hides empty/missing directories during its normal scan. Adding games
and reopening ES can therefore reveal a prepared system without rebuilding the
application. A genuinely new core/configuration needs the catalog regenerated.
BIOS requirements, archive formats, playlists, input and performance remain
core-specific; file extensions in a profile are not a gameplay guarantee.

The local Salvia 1.0.10 package yielded **35 profiles and 32 available system
sections**. That includes Sega systems, Atari, Nintendo handhelds/consoles, NEC,
PlayStation, Amiga, Commodore, MSX, Spectrum, DOS and other supplied profiles.
The Cannonball profile is recorded as unavailable because its selected binary
is absent. The 32 sections retain all **34 available launch sources**. Arcade
uses one section containing both FBNeo and MAME directories. Neo Geo Pocket
likewise retains both its FBNeo and Beetle directories/formats in one section.
Each game uses its own source's configured core and Salvia context; a missing
selected core is not silently replaced. Identically named files in different
source directories remain distinct games.

The catalog is driven by `.cfg` files. Carousel/theme identities and scraper
platform IDs are separate: for example, Lynx maps to `atarilynx` and DOS to `pc`
for scraping, while keeping their existing sections. Unknown future profiles
retain browser/launch entries and disable scraping until explicitly mapped.
It inherits upstream labels and declared formats; these have not all been
corrected or validated. Themes without a platform asset use ES's existing
fallback. The online package helper uses this same catalog instead of its old
five-system list; it was syntax-checked but not run against the console during
offline work. Cached snapshots can become stale and must be compared with the
live configuration before any future deployment.

## Multiple sources in one section

Grouped sections use the Xbox-only `xboxRoutes` extension below. The system's
ordinary `path` is the common ROM root; its ordinary extension/command remain
for schema compatibility. Xbox routing uses only the matching route, with no
fallback to the section's primary core. Single-source configurations retain the
standard EmulationStation format and behavior.

```xml
<xboxRoutes>
  <route>
    <path>hdd:/ROMs/fbneo</path>
    <extension>.zip .ZIP</extension>
    <command>salvia fbn "hdd:/Emulators/Salvia/FBNeo.xex" %ROM%</command>
  </route>
  <route>
    <path>hdd:/ROMs/mame2003/roms</path>
    <extension>.zip .ZIP</extension>
    <command>salvia mame "hdd:/Emulators/Salvia/MAME.xex" %ROM%</command>
  </route>
</xboxRoutes>
```

These executable names illustrate the format; generate the actual names from
the local inventory. Route roots must be disjoint descendants of the common
root. Only their matching files are scanned, flattened into one game list and
launched through the existing permission/context checks. Metadata and saved
selection retain full physical paths, so duplicate basenames stay separate.
The new routing is an offline candidate and is not installed on the console
while the single-game return test is pending.

Only NES Tetris launching from ES has user-confirmed playability. Prior games
working through Salvia do not establish every ES launch/return route. New
platforms must be tested individually; general game launching remains gated.
