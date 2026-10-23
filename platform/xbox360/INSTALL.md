# Installing a development build

This is an ordinary app-folder installation for an Xbox 360 already able to run
unsigned XEX files. The tested environment is Trinity, dashboard 17559, temporary
XeUnshackle with an already-loaded DashLaunch runtime, Salvia 1.0.10 and Aurora.
This guide does not install an exploit or configure automatic startup. See
[current results](README.md#current-state-2026-10-22) before treating another
emulator/platform as verified.

## Folder layout

Use these exact HDD paths for the current session-return implementation:

```text
Hdd1:/Apps/EmulationStation/
  emulationstation.xex
  cacert.pem
  .emulationstation/
    es_settings.cfg
    es_systems.cfg
    themes/
      simple/
        simple.xml
        readme.txt
        art/                 (and the theme's system folders)
    gamelists/               (optional metadata/artwork references)
      nes/gamelist.xml

Hdd1:/Apps/Aurora/Aurora.xex
```

Aurora calls the HDD `Hdd1:`; the native app uses `hdd:` for the same partition.
`game:` is the running app's directory. The current return target and its private
`SessionReturn` directory are fixed to `Apps/EmulationStation` on the HDD; renaming
the XEX or running it from USB does not provide the same return behavior. The
menu's Aurora destination is also fixed to the path above.

Copy the two [build outputs](BUILDING.md#dependencies-and-native-application)
into the app folder. Built-in fonts/icons are embedded in the executable;
external themes need their own complete asset trees. For the tested Simple theme,
use [RetroPie's es-theme-simple](https://github.com/RetroPie/es-theme-simple/tree/5a6c1daf965b9d410398243c232a34911dad8826),
revision `5a6c1daf965b9d410398243c232a34911dad8826`, in
`.emulationstation/themes/simple`. Preserve its `readme.txt` and attribution to
Nils Bonenberger under its CC-BY-NC-SA terms. Theme assets are not included in
this source repository. Use the theme contents, not its `.git` directory.

The accepted TV installation uses a local Simple-theme variant with
regular-weight metadata labels and darker system headings. A stock copy will
have the original theme styling; the executable's image/font filtering still
applies.

## Systems and Salvia integration

Install/configure Salvia separately and verify it can play your games through its
own browser. ES uses its existing cores, configurations and ROM folders. It does
not provide those files. A minimal `.emulationstation/es_systems.cfg` is:

```xml
<systemList>
  <system>
    <name>nes</name>
    <fullname>Nintendo Entertainment System</fullname>
    <path>hdd:/ROMs/nes</path>
    <extension>.nes .NES</extension>
    <command>salvia nes "hdd:/Emulators/Salvia/Nestopia.xex" %ROM%</command>
    <platform>nes</platform>
    <theme>nes</theme>
  </system>
</systemList>
```

Adjust paths to the actual installation. `salvia nes` selects the `nes`
configuration context; the executable must be that context's selected core.
Salvia's `default.xex` browser does not dispatch an incoming ROM to another core.
The known 1.0.10 `menu.tmp` layout (8,496 bytes), `salvia.cfg` and required
`config/*.cfg` files must exist. Start Salvia normally first if menu state has
not been created. ES preserves a `menu.es-backup` and prepares the context before
handoff. A missing predecessor configuration or unknown layout blocks the launch
instead of guessing an index.

For all available systems, generate a catalog from a **local** Salvia inventory:

```sh
python3 platform/xbox360/system_catalog.py \
  --salvia /path/to/local/Salvia --output /path/to/new/offline-catalog
```

The local folder needs `salvia.cfg`, `config/*.cfg` and the actual core filenames.
The tool writes `catalog.json`, `es_systems.cfg` and `salvia.cfg.proposed` into a
new output folder. It makes no console connection. Its default output assumes
`hdd:/Emulators/Salvia` and `hdd:/ROMs`; inspect/adapt paths before installation.
Compare the proposed Salvia context list with the current console file and append
needed contexts without reordering existing ones or replacing unrelated live
settings from an old snapshot. Merely adding an ES entry does not enable a missing
Salvia context. See [platform coverage and grouped Arcade routes](PLATFORMS.md).

All available definitions are included even when no games are present. Empty
systems stay hidden until ES finds supported files. Metadata can live in
`.emulationstation/gamelists/<system>/gamelist.xml`, with artwork paths referencing
existing assets; it is optional for a text list. Keep full physical game paths
when grouping multiple backends in one section so same-named files remain distinct.

## Settings and a combined manual check

For a new `.emulationstation/es_settings.cfg`, begin with the following XML
fragment. ES settings are sibling elements with **no wrapping root element**:

```xml
<bool name="XboxLaunchEnabled" value="false" />
<bool name="XboxSessionReturn" value="false" />
<string name="XboxTestRom" value="" />
<int name="XboxDiagnosticFrames" value="0" />
<bool name="XboxDiagnosticBrowse" value="false" />
<bool name="XboxDiagnosticHandoff" value="false" />
<bool name="EnableSounds" value="false" />
<string name="ThemeSet" value="simple" />
```

On an existing installation, edit only the intended settings and retain input,
selection, metadata and other preferences. ES supplies Xbox controller defaults.
The initial settings permit browsing and menu exit but block game launches.

Once browsing works and the Salvia paths/context files have been checked, enable
`XboxLaunchEnabled` for a manual library test and set `XboxSessionReturn` to
`true` to test returning to ES. Keep `XboxTestRom` empty for a library test.
For a single-game test instead, leave `XboxLaunchEnabled=false` and set
`XboxTestRom` to that game's absolute path. Neither setting is a compatibility
claim. Session return requires the existing DashLaunch API and writable app-local
storage; it does not install DashLaunch. See [return behavior](SESSION_RETURN.md).

Prepare one representative game per core and check them in one TV session:

1. Open `Hdd1 -> Apps -> EmulationStation -> emulationstation.xex` from Aurora's
   file manager. Select a system with Left/Right, enter with A, select with
   Up/Down and launch with A. B goes back.
2. Play briefly, checking picture, controls and sound. Press Xbox Guide, then Y
   (labelled **Xbox Home**) and release Y. Check that ES returns with the same
   game selected. Repeat for each core; leave the least-tested core until last.
3. Finish with ES **Start -> Quit -> Return to Aurora -> Yes**. Report all results
   together, including failures and the final screen. On a crash or stuck screen,
   stop the batch rather than repeatedly relaunching the failing game.

Salvia's Back+Start returns to its own browser; it is different from the Guide/Y
route above. Only the documented NES Tetris configuration has completed the full
gameplay/return/runtime-restoration check so far. The current six-core batch is
pending. A family-facing automatic startup or disc/native Xbox game flow is not
established by this installation.

## Updates, evidence and removal

Transfer files while Aurora is open and ES/emulators are not in use. Before an
update, save the existing app/configuration files, replace only the intended
files and read back their contents/hashes. Preserve `.emulationstation` and
`SessionReturn` records rather than copying another console's state or resetting
preferences. No root `launch.ini` edit is part of this procedure.

Before reopening ES after a test, preserve `.emulationstation/es_log.txt` and
`emulationstation.log` beside the XEX, plus relevant settings/selection/return
records. Logs and return snapshots can contain private console paths/settings;
keep raw copies private. A successful return to Aurora is an observation, while
exact runtime restoration needs the separate comparison described in
[SESSION_RETURN.md](SESSION_RETURN.md).

For removal, first finish through ES's **Return to Aurora** action and resolve
any restoration error. Do not delete its restore records during an active return
session. Once the prior runtime configuration is restored and Aurora is open,
remove only the added `Apps/EmulationStation` folder if desired. Existing
emulators, ROMs and saved startup settings are independent of that folder.
