# Salvia is a separate installation

Reference: [Salvia's README at v1.0.10](https://github.com/danibiker/Salvia/blob/v1.0.10/README.md)
and [v1.0.10 release](https://github.com/danibiker/Salvia/releases/tag/v1.0.10),
reviewed 2026-10-22. This package contains no Salvia executable or source, core,
BIOS, ROM, artwork, music or controller profile.

1. Obtain `Salvia-360.zip` from the upstream release, not the Windows archive.
   Its GitHub-published SHA-256 is
   `7b135e1708a5add04616ed1d797357e4e9badb249f662ff28f6d706a9944fc98`.
   The local archive used to validate the ES catalog matched that digest.
2. Extract its contents into `Hdd1:/Emulators/Salvia`. Keep `default.xex`, core
   XEX files, `salvia.cfg`, `config`, `data`, `assets`, `system` and other upstream
   support directories together. Do not copy only default.xex. Salvia allows
   HDD or USB; this guide chooses HDD to match the included ES configuration.
3. In Salvia's `salvia.cfg`, change the existing `roms_path` to `Hdd:\ROMs`.
   Upstream defaults to `usb:\roms`. Keep `path_prefix=game:\`,
   `libretrosystem=game:\system`, and its app-local save/state paths.
   Retain its full `emulators` list in the original order and all `config/*.cfg`.
   ES selects a named context before launching its core; missing earlier configs
   can change that positional selection and are deliberately rejected.
4. Put your own games into the corresponding folders under `Hdd1:/ROMs`.
   The package's `ROM-FOLDERS.csv` lists every selected profile/core/folder;
   examples are `nes`, `snes`, `gameboy`, `n64`, `fbneo` and `mame2003/roms`.
   Arcade sets must match the chosen core. FBNeo and MAME share one ES Arcade
   section but retain different files and launch commands.
5. Supply any required BIOS files yourself under Salvia's `system` directory,
   following that core's documentation. Neither this release nor its setup tools
   downloads them. The N64 release specifically requires `RiceVideo.cfg`,
   `RiceVideoLinux.ini`, and `mupen64plus.ini` from Salvia's archive in `system`.
6. Start `Hdd1:/Emulators/Salvia/default.xex` normally and test a game there.
   Exit through Salvia's normal menu to save `menu.tmp` beside its XEX files. The supported Xbox
   layout is 8,496 bytes; it is absent from a fresh upstream archive. If this file
   is missing, start/exit Salvia before testing an ES launch. Never copy a saved
   menu file from another user or create an empty substitute.

Back+Start leaves a running game for Salvia's browser. Salvia hotkeys and per-game
controls remain its own settings; ES does not replace `retropad.ini` or `.joy`
profiles. No mouse/debug plugin needs to be installed for this frontend.

Already customized Salvia? Preserve it. Use the optional offline catalog helper
against a local copy of your own configuration and core filenames. Compare any
generated context proposal before an intentional change; don't overwrite live
settings with the stock examples. ES compatibility is pinned to this version's
menu structure, not promised for future Salvia releases.
