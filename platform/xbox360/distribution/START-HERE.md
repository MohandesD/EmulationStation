# EmulationStation for Xbox 360 — first installation

This package contains the Xbox application. You do **not** need a compiler or
Microsoft's XDK. Salvia, Aurora, games and BIOS files are separate. This is a
development preview for an Xbox 360 that can already run unsigned homebrew.
It does not modify a stock console or make EmulationStation start at power-on.

## 1. Get the right downloads

Use `EmulationStation-Xbox360-0.1.0-preview.1.zip` from the
[Xbox fork's release page](https://github.com/MohandesD/EmulationStation/releases/tag/xbox360-v0.1.0-preview.1).
The Windows/Linux downloads in the inherited desktop project documentation are
**not Xbox downloads**. GitHub's automatic Source code archives are for developers.

Extract the ZIP on your PC. Keep `START-HERE.md`, `SHA256SUMS.txt` and the included
license/source notices. The `Apps` directory is the part to copy to the console.
Optional: in PowerShell, `Get-FileHash .\EmulationStation-Xbox360-0.1.0-preview.1.zip -Algorithm SHA256`
can be compared with the release's separate checksum file. Checksums verify
contents; they are not a publisher signature.

## 2. Prepare Salvia separately

Download **Salvia-360.zip**, version **1.0.10**, from
[Salvia's own release](https://github.com/danibiker/Salvia/releases/tag/v1.0.10).
Do not use Salvia-win32.zip. Read [SALVIA-SETUP.md](SALVIA-SETUP.md) for its exact
layout, ROM folders and first-run requirement. The shipped ES catalog targets
the selected cores and configuration names in that release.

For the supplied layout, Salvia goes in `Hdd1:/Emulators/Salvia`, and your own
games go under `Hdd1:/ROMs`. At least one supported game must exist: empty systems
are hidden, and an entirely empty library produces the upstream "no systems"
message. There are no demonstration ROMs in this package.

First verify one game through Salvia's own browser and exit normally to Aurora.
This also creates Salvia's required `menu.tmp` beside its XEX files (8,496 bytes). ES does not
invent or ship another user's menu state. No controller profile is included.

## 3. Copy the app folder

From Aurora's file manager or FTP, copy the entire `Apps/EmulationStation` folder
to the HDD's `Apps` directory, including the dot-prefixed `.emulationstation`
folder. Copy while the app/emulators are closed. The result must be:

```text
Hdd1:/Apps/EmulationStation/emulationstation.xex
Hdd1:/Apps/EmulationStation/cacert.pem
Hdd1:/Apps/EmulationStation/.emulationstation/es_systems.cfg
Hdd1:/Apps/EmulationStation/.emulationstation/es_settings.cfg
Hdd1:/Apps/EmulationStation/.emulationstation/themes/clean/...
Hdd1:/Apps/EmulationStation/licenses/...
Hdd1:/Apps/Aurora/Aurora.xex                    (your existing Aurora)
Hdd1:/Emulators/Salvia/default.xex              (installed separately)
Hdd1:/ROMs/nes/Your game.nes                    (your own game)
```

Aurora's `Hdd1:` and ES's `hdd:` refer to the same HDD partition. Salvia spells
it `Hdd:`. In a configuration, `game:` means the running executable's folder.
The Aurora menu destination and ES session-return destination are fixed to the
paths above. Do not rename the XEX or use USB as the app's final location for
this preview. A USB stick is fine for transferring the folder to HDD.

**Existing installation:** do not copy this starter configuration over your
settings, gamelists, input mappings, themes, selection or `SessionReturn` records.
Back up your app folder and update only the intended files after ending its
return session normally. In particular, this release should not replace an
already-working family installation just to try the packaging.

## 4. Browse, then enable playing

Open **Aurora File Manager → Hdd1 → Apps → EmulationStation → emulationstation.xex**.
Left/Right selects a system; A enters; Up/Down selects a game; B goes back; Start
opens the menu. The supplied original Clean theme uses text labels and embedded
fonts, without console logos, music or downloaded artwork. Other compatible
EmulationStation format-3 themes remain supported as external files.

Initially games are disabled. After checking the folders and Salvia first run,
exit via **Start → Quit → Return to Aurora → Yes**. Edit the app's external
`.emulationstation/es_settings.cfg` in a text editor:

```xml
<bool name="XboxLaunchEnabled" value="true" />
<bool name="XboxSessionReturn" value="true" />
```

Change the two existing values; do not add duplicates or a wrapping XML element.
Keep `XboxTestRom` empty and the diagnostic options off. Reopen the app and play
one game from each core you intend to use in one test session.

Session return requires an **already-loaded compatible DashLaunch runtime** and
writable HDD storage. It was tested with temporary XeUnshackle on Trinity/17559.
This ZIP does not install that runtime, an exploit, XBDM or a startup plugin.
Other softmods and hardware are not established by that test. With session return
disabled, the console's existing Home destination applies instead.

While playing, **Xbox Guide → Y (labelled Xbox Home), then release Y**, returns
to ES with the selection retained. **Back+Start** returns to Salvia's own browser.
To finish the ES session, use **Start → Quit → Return to Aurora → Yes** in ES;
this restores its previous runtime settings. No saved `launch.ini` is changed.
Keep the existing network isolation and security settings; no internet access is
needed to browse or play local games.

## If something fails

| Symptom | Check |
| --- | --- |
| No systems found | At least one real game exists at a configured path with a supported extension. This is expected with an empty library. |
| Game launch disabled | Set the existing XboxLaunchEnabled value to true after setup. |
| Salvia context/menu error | Use the supported 1.0.10 files together; start/exit Salvia once; retain all predecessor configs in its emulators list. |
| Core not found | Check the exact XEX name in es_systems.cfg against your Salvia folder. |
| Return runtime unavailable | The required DashLaunch API is not loaded/compatible; this package cannot supply it. Leave session return off until resolved. |
| Black screen, crash, or restore error | Stop the test; retain the ES logs and SessionReturn records. Do not repeatedly launch the failing game or delete recovery state. |

Logs are `emulationstation.log` and `.emulationstation/es_log.txt`. Review logs
and return snapshots for private paths/settings before sharing. A normal menu
exit should work; after a failure, a power cycle ends a temporary exploit session
and requires its usual manual activation again. Do not edit startup settings to
work around a failed test. For removal, end the return session successfully first,
then remove only the added EmulationStation folder.

## What's tested, and what isn't

The preceding visual build ran on Trinity/17559. The user reported working games
across NES, SNES, Game Boy, FBNeo, MAME 2003 Plus and Nintendo 64; detailed NES
return and final runtime restoration were verified.

This release rebuild adds a compile-time Eigen license guard; the new package
and original Clean theme are checked locally from fresh extraction. They have
**not** been installed on the working console. Local tests do not establish
fresh-install TV appearance, gameplay on every core, sustained performance or
multiplayer. Automatic startup and native Xbox 360 disc/HDD games remain outside
this preview. See [VALIDATION.md](VALIDATION.md) for exact release evidence.

For other layouts, [CONFIGURATION.md](CONFIGURATION.md) explains the optional
offline catalog helper. All configurations, themes and metadata remain external
and editable. EmulationStation is a frontend, not an emulator.
