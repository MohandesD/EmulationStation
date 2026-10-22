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
is absent. Arcade uses one section, with FBNeo and MAME retained as alternative
backends. Neo Geo Pocket's alternative cores likewise do not create duplicate
sections. Backend choice is explicit; a missing selected core is not silently
replaced. Mixing per-game arcade backends in one collection remains future work;
the current section retains FBNeo and its existing game directory.

The catalog is driven by `.cfg` files, with a small alias table for theme/scraper
names. It inherits the upstream labels and declared formats; these have not all
been corrected or validated. Themes without a platform asset use ES's existing
fallback. The online package helper uses this same catalog instead of its old
five-system list; it was syntax-checked but not run against the console during
offline work. Cached snapshots can become stale and must be compared with the
live configuration before any future deployment.

Only NES Tetris launching from ES has user-confirmed playability. Prior games
working through Salvia do not establish every ES launch/return route. New
platforms must be tested individually; general game launching remains gated.
