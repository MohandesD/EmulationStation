# Temporary emulator return: offline candidate

Status on 2026-10-22: implemented, cross-built and PC-tested; **not installed or
executed on the Xbox**. The console is reserved for family play. Tetris already
starts and plays from the previous build, according to the user. That success
does not establish return behavior or support for other cores.

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
existing liveblock/livestrong/noupdater values. Failed verification attempts a
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

The intended game exit is **Xbox Guide -> Y**, followed by releasing Y. Salvia's
**Back+Start** is an internal return to its own browser and remains unchanged.
This code does not intercept that emulator shortcut, alter Salvia binaries or
establish disc/native Xbox 360 game support. Those are separate acceptance items.

## Offline evidence and later console acceptance

Host tests cover snapshot serialization, all-option preservation, repeated game
entry without overwriting the original, missing/corrupt restore state, failed
staging, concurrent changes, reload mismatch, successful rollback and failed
rollback. These are injected runtime tests, not a hardware result.

Once console access is authorized again, first inspect the actual runtime
snapshot and compare the staged files without launching a game. Then perform a
bounded no-game arm/restore check, verifying unchanged saved boot files. Only
after that passes, repeat the already-working single Tetris test, use Guide -> Y,
confirm the selected game is restored, then Return to Aurora and confirm the
original runtime destination. Keep broad game launching disabled during this
check. Repeat launch/return separately for other cores before enabling them.

## Source basis

Reviewed DashLaunch revision `f57eca9ba6dbba15dbdb226059fa94ad0dfaee98`:

- [Export ABI](https://github.com/XeFreedom/DashLaunch/blob/f57eca9ba6dbba15dbdb226059fa94ad0dfaee98/ReadMe/launch_sysdll_exports.c): ordinals 4, 5, 6 and 11; 272-byte path structure.
- [Runtime reload](https://github.com/XeFreedom/DashLaunch/blob/f57eca9ba6dbba15dbdb226059fa94ad0dfaee98/dll/launch.c): `runtimeRunTasks`, path resolution and reload fallback.
- [Options](https://github.com/XeFreedom/DashLaunch/blob/f57eca9ba6dbba15dbdb226059fa94ad0dfaee98/dll/opts.c): reload resets omitted values; the ordinary setter does not implement path updates.
- [INI parser](https://github.com/XeFreedom/DashLaunch/blob/f57eca9ba6dbba15dbdb226059fa94ad0dfaee98/dll/iniparse.c): global key lookup, boolean/hex values and restricted path character set.

The loaded temporary environment may differ from this upstream revision; that is
why actual reload acceptance remains pending despite a previously successful
read-only API probe.
