# Temporary emulator return: Tetris roundtrip confirmed

Status on 2026-10-22: cross-built and PC-tested; a no-game console arm/restore
check now passes on Trinity/17559 with temporary XeUnshackle. All 76 exposed
options were restored and the saved boot file stayed unchanged. The user has now
confirmed ES -> NES Tetris plays -> Xbox Guide/Y -> ES, with Tetris still selected.
An independent read-only XBDM observation confirmed ES running afterward. Manual
Return to Aurora and verification of the current session's runtime restoration
remain pending; the earlier no-game restoration result is a separate check.

## Behavior

`XboxSessionReturn` defaults to `false`. When explicitly enabled for a future
test, an approved game launch prepares a return to
`hdd:/Apps/EmulationStation/emulationstation.xex` before dismantling the UI.
The existing game-launch and single-ROM gates still apply. This candidate only
supports that HDD installation; it is not a USB-portable return implementation.

The already-loaded DashLaunch runtime is queried through its documented exports.
All exposed numeric/path options are copied into a validated snapshot. The
original is saved and read back under the application's `SessionReturn` folder.
Two private INIs represent original and active settings; only `Default` and
`BUT_Y` differ. No root launch.ini, NAND, persistent exploit or boot installation
is written. Runtime reload does not start boot plugins or apply boot patches.

Before reload the snapshot is checked again. After reload every exposed option
is compared with the intended result, including path device/root/flags and the
existing liveblock/livestrong/noupdater values. One precisely documented metadata
transition is accepted after reload: a plugin path's PLUGIN_LOADED (0xFB) becomes
ITEM_FOUND (0xFE). The path, device, root and all other values must match; missing
plugins, altered paths and reverse/unrelated flag transitions fail. Snapshot
persistence and the pre-reload race check still require strict equality. The
runtime resets this marker without unloading or reloading the resident plugin.
Failed verification attempts a
verified restoration and blocks the launch. Unknown types, unsafe-to-serialize
paths, failed writes and missing restore state are rejected. API absence does
not fall back to raw pointer writes or a persistent configuration change.

An arriving ES process recognizes the active snapshot and retains the original
restore point. The existing selection file supplies the selected system/game.
Choosing Return to Aurora restores the previous session settings before leaving
the UI. Failed native handoffs also attempt restoration. If another application
changes the runtime options in the meantime, the code refuses to overwrite that
unexpected state. Restoration errors remain visible and require diagnosis or a
fresh temporary session. A runtime reload is not atomic and rollback is best
effort; PC tests cannot prove the console API's behavior.

The next offline revision checks the current options again after staging the
restore INI, immediately before reload. A concurrent change cancels restoration
without overwriting it. Failed handoffs also pass the specific recovery error
back to the game-list dialog instead of showing only a generic launch failure.
These follow-up safeguards are PC-tested separately from the currently installed
manual-test build.

The diagnostic frame-limit exit also restores the previous session before
leaving the frontend. If restoration fails, it stops automatic browsing/captures
and shows the error once while keeping the UI open. The ordinary Quit action
can retry recovery. This closes the diagnostic-only exit path that previously
bypassed the restoration used by the menu; the installed manual test has
diagnostics disabled.

The tested game exit is **Xbox Guide -> Y**, followed by releasing Y. The Guide
still labels Y **Xbox Home**: the label is unchanged while the temporary
`Default`/`BUT_Y` settings route that action to EmulationStation. Salvia's
**Back+Start** is an internal return to its own browser and remains unchanged.
This code does not intercept that emulator shortcut, alter Salvia binaries or
establish disc/native Xbox 360 game support. Those are separate acceptance items.

## Offline evidence and later console acceptance

Host tests cover snapshot serialization, all-option preservation, repeated game
entry without overwriting the original, missing/corrupt restore state, failed
staging, concurrent changes, reload mismatch, successful rollback and failed
rollback. These are injected runtime tests, not a hardware result.

The read-only native probe first captured 76 options and verified the target.
The first no-game reload exposed the plugin-status transition above: restoration
returned every configuration value and Aurora's default but the old comparator
rejected the marker. XBDM stayed responsive. Tagged upstream source explains this
behavior; regression tests now model a loaded plugin and an unset BUT_Y.

The corrected probe then armed ES and restored Aurora successfully, with a
byte-identical before/after option snapshot in that run. Independent FTP reads
confirmed only Default/BUT_Y differ between staged INIs and that the root boot
file is unchanged. The probe was returned to inspect mode and renamed .checked.
See [console evidence](tests/hardware-session-return-2026-10-22.json).

The installed app has been read-back verified with XboxSessionReturn=true,
XboxLaunchEnabled=false and the single Tetris exception. Its source revision is
`d94d2fac6ed7cfaac6184831ce069b67e9fda9bb`; XEX SHA-256 is
`a860c391eca0b1e24cbe4e7e5f5d1d06785050bf104c2f644488c87176503fbb`.
The user operated the game and confirmed it played, Guide/Y returned to ES, and
Tetris was selected. Read-only XBDM independently confirmed the frontend running;
it does not establish which controls the user pressed or the selected entry.
See [Tetris return evidence](tests/hardware-tetris-return-2026-10-22.json).

Next verify manual Return to Aurora and original runtime restoration after this
gameplay roundtrip. Repeat launch/return separately for other cores before
enabling them. Font appearance still needs the user's TV check. Newer offline
safeguards and grouped-source changes have not been installed or hardware-tested
by this result.

## Source basis

Reviewed DashLaunch revision `f57eca9ba6dbba15dbdb226059fa94ad0dfaee98`:

- [Export ABI](https://github.com/XeFreedom/DashLaunch/blob/f57eca9ba6dbba15dbdb226059fa94ad0dfaee98/ReadMe/launch_sysdll_exports.c): ordinals 4, 5, 6 and 11; 272-byte path structure.
- [Runtime reload](https://github.com/XeFreedom/DashLaunch/blob/f57eca9ba6dbba15dbdb226059fa94ad0dfaee98/dll/launch.c): `runtimeRunTasks`, path resolution and reload fallback.
- [Options](https://github.com/XeFreedom/DashLaunch/blob/f57eca9ba6dbba15dbdb226059fa94ad0dfaee98/dll/opts.c): reload resets omitted values; the ordinary setter does not implement path updates.
- [INI parser](https://github.com/XeFreedom/DashLaunch/blob/f57eca9ba6dbba15dbdb226059fa94ad0dfaee98/dll/iniparse.c): global key lookup, boolean/hex values and restricted path character set.

The loaded temporary environment may differ from this upstream revision; that is
why the successful no-game reload check alone does not establish gameplay exit.
The user-confirmed gameplay result covers the installed NES Tetris configuration;
compatibility with other cores and softmods remains unverified.

Plugin marker constants: [`_common.h`](https://github.com/XeFreedom/DashLaunch/blob/f57eca9ba6dbba15dbdb226059fa94ad0dfaee98/_common.h).
