# EmulationStation for Xbox 360 — working development port

This branch ports **Aloshi/EmulationStation itself**, based on
`9cc42adff67946175d2b7e25c6ae69cc374e98a0`. The final target is a native `.xex`
running the existing system carousel, game lists, themes, artwork and controller
navigation, with emulator launch and a verified way back to the frontend.
The earlier custom Games dashboard is a separate project, not this port.

Start with the [build guide](BUILDING.md) and [installation guide](INSTALL.md).
For installation without a compiler, use the [download package guide](distribution/START-HERE.md).
See [platform coverage](PLATFORMS.md) and [temporary game return](SESSION_RETURN.md)
for integration details. Dated evidence below distinguishes past tests from the
current installed build.

## Current state (2026-10-22)

The working-port/public-fork objective is accepted with the scoped evidence in
[ACCEPTANCE.md](ACCEPTANCE.md). The user deferred EBA X-Men's controller issue;
its mapping stays unchanged. Automatic startup and native Xbox game integration
remain separate follow-ups, with no persistent installation performed.

A visual follow-up uses linear magnification for non-tiled Xbox images. The
local Simple-theme variant uses regular-weight metadata labels and darker system
headings, preserving the menu/game-name styles the user accepted. The native
build, three targeted graphics checks and four package checks pass. Twenty-five
files were deployed with rollback copies/read-back verification while fourteen
protected files stayed unchanged. The user accepted visual build `1222c78c167d`
on the TV, reporting that grey labels and system logos both look better.
The detailed single-Tetris roundtrip result belongs to the earlier
`3e3d7dd490bb` build. The visual build now also has the six-game user report and
independent post-batch cleanup result below.
See [visual-candidate evidence](tests/image-readability-candidate-2026-10-22.json).

At the user's request, the installed library is now enabled for a single manual
test session across NES, SNES, Game Boy, both Arcade backends and Nintendo 64.
All 44 current launch commands passed the production parser/context validation
against fresh archived configuration; listed game/core files exist and are
nonempty. The missing MAME context was appended without reordering existing
entries. Two configuration files were backed up and read-back verified, with
48 protected files unchanged. The accepted application/theme were not replaced.
This preflight does not prove emulator gameplay. The user subsequently reported
that all six games worked, with a control misconfiguration in MAME's EBA X-Men
entry. Aurora was independently observed before inspection. Archived logs show
restored game-list selection and clean exit to Aurora, with Kirby 64 selected.
The inspect-only probe and production comparator verified all 76 runtime options
byte-identical to the known original baseline. Saved boot settings are unchanged,
and the probe is disabled. No game was remotely launched. EBA's specific incorrect
buttons remain undescribed; the user subsequently deferred this issue, and no
controls were changed.
The reply did not separately describe each game's controller actions, sound or
selected-game return. See [batch setup](tests/manual-library-batch-2026-10-22.json)
and [hardware results](tests/hardware-library-batch-2026-10-22.json).

The graphics component cross-compiles into `ESGraphicsProbe.xex` and has run on
the Trinity/17559 console. A native 1280x720 capture shows correct RGB blocks,
alpha blending, scissors and texture colours; 14 pixel checks and a native line check pass. After 1,800
frames the diagnostic automatically returns to HDD Aurora, observed via XBDM.
The full C/C++ toolchain and real es-core/es-app now cross-build successfully.
The native application has loaded five system lists (43 existing games), rendered
the upstream Simple theme's system carousel at 1280x720 and returned cleanly to
Aurora after a bounded 600-frame test. Its own fenced capture was visually checked.
An additional 1,200-frame diagnostic visited all five real game lists. Native
captures show their system headers, game names and selected cover art; all counts
match the installed library. This automated rendering check does not prove input.
Later user-operated tests passed controller navigation, menu return to Aurora
and restoration of the selected system. The source is published on the
[Xbox branch of the maintained fork](https://github.com/MohandesD/EmulationStation/tree/xbox360).
The first manual Tetris handoff crashed after the emulator appeared. The candidate
Salvia context correction below passed a no-game console check. A subsequent
read-only XBDM capture shows Tetris rendering a game, and the user confirmed it
starts and plays. The user has also confirmed Guide/Y returns from Tetris to ES
with Tetris selected; read-only XBDM independently confirmed ES running afterward.
The Guide still labels Y "Xbox Home" while the temporary session routes it to ES.
A no-game session arm/restore check passed earlier. Return to Aurora opened Aurora,
but the post-game read-only check still found Default/BUT_Y pointing to ES. The
other 74 options and saved boot matched. A separate exit-only menu check with the
same installed app then restored all 76 options exactly, with a clean application
exit, the expected HDD image in the SDK log and an unchanged saved boot file.
The earlier post-game mismatch remains unexplained; its evidence is preserved in
[the earlier return record](tests/hardware-tetris-return-2026-10-22.json).
The newer catalog build has now passed the complete Tetris sequence, including
menu exit to Aurora and exact post-game restoration of all 76 runtime options.
Saved boot settings remained unchanged. This earlier, detailed single-game result
is preserved separately from the newer batch report above. See
[Tetris return evidence](tests/hardware-tetris-return-catalog-2026-10-22.json).
The port is a working development build with deferred EBA controls and limits on
broader sustained/platform testing, as recorded in the acceptance document.

The newer catalog candidate has now passed a 1,200-frame browser-only console
check: 32 configured sections route 34 emulator sources, and the five populated
lists contain 44 entries. Existing metadata was preserved. It exited cleanly to
Aurora and its temporary diagnostic settings were restored exactly. On that
build, the user accepted game-name and ES menu text but reported fuzzy grey labels
and unsmoothed system logos. The later visual update was accepted separately.
See [catalog evidence](tests/hardware-catalog-2026-10-22.json).

`es_gl.c` implements the 26 fixed-function GL calls used in the upstream source.
It preserves the existing renderer/components and implements column-major model
and projection transforms, separate/interleaved vertex arrays, byte-order-safe
colours, alpha font atlases, partial texture updates, blend factors and scissors.
`es_gpu.c` submits those draws to Xenos through the open-source xsdk, using
fenced vertex/texture lifetime management. This is a bounded ES compatibility
layer, not a general OpenGL driver. Unsupported calls/parameters report errors.
Limits are 1,023 named textures, 8,192 pixels per texture side, 64 MiB per CPU
texture image and 65,536 vertices per draw; large frames drain the vertex pool.
There is no depth-tested 3D path or mipmapping; upstream ES uses neither.

`Port.cmake` now wires the real es-core/es-app targets to the native renderer,
SDL3 controller/audio adapters, FreeType, stb images, libcurl/mbedTLS and C++17
filesystem. A host type/API check passes for 70 actual application/adapter C++
translation units. This check is not a console cross-link or runtime test.
The separate native C++ diagnostic also passes filesystem creation/readback,
directory iteration, canonical paths, streams and exception throw/catch, followed
by a verified return to Aurora. The Xbox menu provides Return to Aurora, hides unsupported shell shutdown
actions, and defaults to quiet operation. Controller defaults use SDL-xenon's
actual button ordering; loaded user mappings override them.

Generic native commands have the form `xex "hdd:/path/app.xex" "hdd:/path/game.nes"`.
Salvia commands require their explicit context, for example
`salvia nes "hdd:/Emulators/Salvia/Nestopia.xex" %ROM%`; the generic form is
rejected beside a Salvia configuration. Commands are parsed as paths, never
sent to a shell. Salvia ROM data uses its own
`Hdd:`/`Usb0:` mounts. `XboxLaunchEnabled` defaults to false for a new installation.
It is now enabled on the test console. The combined six-game report is positive
apart from EBA X-Men's controls, and post-batch runtime cleanup passed.
Session return is implemented; its no-game console check and a user-operated NES
Tetris roundtrip with selected-game restoration pass. The current catalog build
also passed post-game menu exit with exact restoration of all 76 options, as did
the newer batch. The earlier build's mismatch remains documented; neither result
establishes every game's compatibility or sustained stability.

The hardware probe caught an actual backend defect: the vertex-stream API
expects the complete buffer byte length, not the per-vertex stride. The adapter
now supplies all vertices and writes GPU memory with volatile 32-bit stores.
XBDM's D3D screenshot metadata was stale for this native renderer, so the probe
captures its own fenced render target instead. Do not use the earlier blank
XBDM crop as evidence of the corrected renderer's output.

## Reproducible component checks

SDK: https://github.com/xenosworks/xsdk at
`57b02f1e60667e37d22c3aa613d23d2486514ff8` (MIT and included third-party notices).
Compiler: patched powerpc-xsdk-elf GCC 15.3.0, newlib 4.6.0.20260123,
binutils 2.47, built with that revision's `tools/toolchain/build.sh`.
The SDK's patches match the already working C-only compiler from the earlier
dashboard. The separate full C/C++ toolchain completed its built-in ABI checks;
the installed C-only compiler remains intact.

The [build guide](BUILDING.md) defines the paths, pins the SDK/toolchain and
provides commands for host tests, the full application and optional probes.
It explicitly passes the selected compiler to the SDK Makefile. C17 builds use warnings as
errors. Host tests enable address/undefined-behaviour sanitizers and keep asserts
enabled in release builds. Tests check actual ES array formats, nonzero draw
offsets, unaligned/interleaved floats, projection/translation, alpha atlas padding
and subupdates, raw RGBA order, clipping state, texture lifetime and errors.
The source inventory fails if upstream adds an unimplemented GL entry point.
Compiled shader fetch records must match POSITION0/COLOR0/TEXCOORD0 and sampler
slot zero; a deliberately wrong sampler is rejected by the validation script.

Nine host test groups cover GL semantics/source coverage, platform catalogs,
Xbox launch paths, native handoff recovery, grouped routes, Salvia context,
session return and image orientation/alpha/resize/failure handling. Native
host tests run with address/undefined-behaviour sanitizers enabled. The optional
`ES_MODEL_DEPS` target additionally compiles the production configuration,
game-list and metadata code against filesystem fixtures. It uses the pinned
Boost/Eigen/SDL/FreeType headers and small presentation stubs, with no console
runtime or UI linked. It checks scanning, per-source commands, duplicate
basenames, metadata isolation, save/reload and game-list-only reconstruction.
They complement the separate graphics-only console evidence.
The SDK's instruction scanner reports three skipped instructions in the small
graphics probe and 164 in the current full application/dependencies. Xenia results in
those functions are unreliable; Xenia is not a substitute for a console run.

## Full application build

Follow [BUILDING.md](BUILDING.md) for host prerequisites, the full C/C++ compiler,
pinned dependencies and the native application build. The resulting runtime
files are `es-app/emulationstation.xex` and `cacert.pem` in the build directory.
The full application has built successfully, and a fresh configure using the
documented command passed on 2026-10-22. It remains a development build with the
runtime acceptance limits recorded above.

## Display bring-up and installation

The [installation guide](INSTALL.md) provides the complete folder layout, initial
settings, a system configuration example and a combined manual test procedure.
The current test installation is an ordinary `Hdd1:/Apps/EmulationStation` folder.
Its `.emulationstation/es_systems.cfg` points at the existing ROM folders; artwork
is referenced from the existing Salvia assets. App-local gamelists keep metadata
separate. No ROMs, saved startup files or persistent modifications are involved.
All 42 package files passed full independent-session FTP readback; later executable
updates were verified separately while preserving ES's updated gamelists.

The stock Simple theme is from https://github.com/RetroPie/es-theme-simple at
`5a6c1daf965b9d410398243c232a34911dad8826`, by Nils Bonenberger, under its
CC-BY-NC-SA terms. Its assets are not vendored in this source branch; preserve its
`readme.txt` and attribution when packaging it. The themes use the original ES
format version 3. A corrected relative-path join supports `./../simple.xml`
under both Boost filesystem and the native C++17 filesystem implementation.

For a bounded display-only run, set `XboxDiagnosticFrames` to 600 in the app's
settings. Frame 120 is captured to `game:/es-frame.raw` after presentation and
before clearing; after 600 main-loop frames the app exits cleanly to Aurora.
The default is zero, which leaves the frontend open normally. For a display-only
run, keep `XboxLaunchEnabled=false` and `XboxTestRom` empty. Hotplugged Xbox pads receive defaults;
an unplugged controller no longer strands startup in the desktop setup wizard.
Physical navigation and the menu's Return to Aurora action passed the later
user-operated test documented below.

`XboxDiagnosticBrowse=true`, together with a positive diagnostic frame limit,
visits successive system game lists every 180 frames starting at frame 180 and
captures each list after its transition settles. It calls view navigation only;
no controller events or ROM launches are injected. A 1,200-frame five-system run
passed. Both diagnostic options are now disabled in the installed package, so a
normal manual launch stays open. The installed frontend permits the library;
the user-reported batch outcome and remaining control issue are recorded above.

Rollback: finish through Return to Aurora and resolve any runtime restoration
error before removing the separately added app folder. Keep restore records
while a return session is active; see [removal](INSTALL.md#updates-evidence-and-removal).
Existing emulators, ROMs, assets and startup settings are independent.
Do not set this development build as the default dashboard.

## Next integration work

### Manual controller check (2026-10-22)

The user confirmed system Left/Right, entering a game list, Up/Down selection,
Back, and Start -> Quit -> Return to Aurora. A second launch restored the user's
Game Boy system selection, also confirmed on the TV. The application's saved
selection and clean-exit/restoration logs corroborate those reports. This tests
restoration of the system screen. The later Tetris test separately confirmed
selected-game restoration after emulator exit.

For a staged manual game test, XboxTestRom may name one absolute ROM path while
XboxLaunchEnabled remains false. Both the UI and native handoff enforce this
restriction; an empty setting permits no games, and it never permits an app-only
launch. The default is empty. This is a test control, not a gameplay success claim.

### Launch preflight and diagnostic evidence

#### Failed Tetris launch and candidate correction (2026-10-22)

The user saw emulator startup followed by "fatal crash intercepted". ES's private
logs show its handoff to Nestopia; XBDM then reported the crash helper. Aurora
was recovered remotely, logs preserved privately, and the test exception cleared.
This matches the earlier custom Games launch symptom, but no exception address
was captured, so an identical crash site is not established.

Read-only inspection found Salvia's saved menu still selected Arcade (index 2),
whose configuration uses `no_uncompress=yes`; NES uses `no_uncompress=no`.
Salvia restores that selection even for incoming launch data, and its loader
reads the selected configuration. This can send Nestopia an empty memory buffer
instead of loading the NES file. This is a concrete integration defect and a
strong crash hypothesis; the subsequent user test confirmed that Tetris starts and plays. Source references:
[menu restoration](https://github.com/danibiker/Salvia/blob/v1.0.10/src/salvia.h),
[configuration selection](https://github.com/danibiker/Salvia/blob/v1.0.10/src/io/cfgloader.cpp),
[Nestopia's memory input](https://github.com/danibiker/Salvia/blob/v1.0.10/libretro/nestopia/libretro/libretro.cpp).
The upstream logger's file write is commented out, so a missing salvia.log does
not establish an incorrect working directory.

Salvia commands now specify their configuration name explicitly:
`salvia nes "hdd:/Emulators/Salvia/Nestopia.xex" %ROM%`.
Use `snes`, `fbn`, `gameboy` and `n64` for the other configured systems. The helper
validates the core association, system list and known v1.0.10 Xbox menu layout
(8,496 bytes). After launch permission, it preserves `menu.es-backup`, selects the
requested system, clears stale directory/filter positions, and verifies the write
before UI shutdown. Layout/background are retained. Missing/malformed inputs or
write failures cancel launch. Legacy ROM commands beside salvia.cfg are rejected
until their system is specified. These checks identify supported structure, not
binary authenticity or future Salvia layout compatibility.

The positional system list must contain nonempty lowercase configuration names
separated by single spaces; tabs and ambiguous/invalid names are rejected.
Every configuration before the requested system must also be readable. Salvia
skips missing configurations, so otherwise the saved index could select a
different system or Options. An empty readable predecessor still occupies its
normal index. A missing later configuration does not shift the target. These
checks happen before changing the menu or backup; they do not remap indices.

Five sanitizer tests pass, including wrong-core/unknown-layout rejection, backup
failure and the Arcade-to-NES regression. A local copy of the captured settings
also passed. ESSalviaContextProbe exercised the same helper on the console without
launching a core or ROM, verified NES selection and exact backup/readback, and
returned to Aurora. Its executable was disabled afterward. The updated app and
five commands passed full FTP readback; saved boot settings are unchanged. Only
Tetris was then re-enabled for a user-operated check. XBDM subsequently reported
Nestopia.xex, and a private 1280x720 framebuffer showed the Tetris playfield, pieces,
score and line counter. The user subsequently confirmed that Tetris starts and plays.
At that checkpoint sustained stability and emulator return were unverified. At the user's request, the
console was then left in Salvia for family play.
See [sanitized evidence](tests/hardware-tetris-2026-10-22.json).

The native launcher checks that the chosen core is a regular XEX2 file and the
game is a nonempty regular file before dismantling the UI. These are basic file
checks, not executable authentication or gameplay compatibility tests. Failed
handoffs restore the frontend and show an error without counting a play. ROM
paths use Salvia's Hdd/Usb0/Usb1/Usb2 mounts; DVD ROM paths are rejected because
this Salvia release does not mount Dvd. This does not implement Xbox disc play.

Commands must select the actual emulator core. Salvia's default browser does not
dispatch incoming ROM launch data to another core. The tested installation's
selected cores are Nestopia.xex, Snes9x2010.xex, fbneo.xex, Gambatte.xex and
xbox64.xex. The preparation helper reads and verifies those selections from the
installed configuration instead of hardcoding default.xex for every system.

Before a game handoff, the frontend records its selected system/game in its own
es_selection.cfg. Startup accepts only an entry that still exists in the loaded
catalogue; malformed or stale state falls back to system selection. The user has
confirmed selection restoration after the Tetris roundtrip. It does not
by itself change where an emulator exits.

Build the small diagnostics with `make -f Makefile.probes` from this directory,
using the same SDK and full C/C++ compiler as the application. A read-only
ESReturnProbe confirmed that the existing DashLaunch return API is available,
the current default is Aurora, and the network/update protection options are
enabled. It invoked no setter or INI reload. See the sanitized evidence JSON.

For the fixed handoff diagnostic only, install ESHandoffReceiver.xex and a text
file named handoff-fixture.txt containing `EmulationStation handoff test` plus
a newline under Hdd1:/Apps/ESProbe. Set XboxDiagnosticFrames=180 and
XboxDiagnosticHandoff=true in the app's settings, retaining XboxLaunchEnabled=false.
After its bounded render loop and UI shutdown, ES passes that text file's path
to the receiver through the same native launch helper used by game commands.
The receiver verifies the exact NUL-terminated launch data, its game: directory,
and an absolute read, then returns to Aurora. This passed on the console; it
does not test a Salvia core, ROM launch or return from an emulator. Both options
were restored to their normal disabled values after collecting the result.

The earlier receiver/read-only diagnostics changed no return settings. The
subsequent session-return probe temporarily changed and restored runtime
Default/BUT_Y, verified all 76 exposed options and left saved boot files
unchanged. See [session return](SESSION_RETURN.md) for its precise behavior,
rollback and scoped gameplay acceptance. PC tests do not establish gameplay or
justify startup changes. The completed manual batch collected the user report
and post-session restoration evidence recorded above.

1. Verify audio, filesystem/locale behavior and text input;
   dummy SDL text input is only an adapter, not a verified Xbox keyboard UI.
2. Extend the passed game-list, cover and input checks to sustained browsing
   and memory lifetime on the Trinity/17559 temporary XeUnshackle session.
3. Extend the verified NES launch/return path to the other emulator cores,
   including controls, sound and post-session restoration. Keep saved boot
   configuration unchanged during these checks.
4. Maintain the published Xbox fork with reproducible builds, notices,
   installation/rollback instructions and explicit hardware results.

The component probe shows RGB blocks, a translucent overlay, a clipped rectangle
and a four-colour texture. B requests an early return to HDD Aurora through
xsdk's documented launcher; an automatic return after 1,800 frames is verified.
The B input itself has not been tested. No ROM is launched by the probe.
Do not set it as the default dashboard.

No ROM, BIOS, commercial artwork, console keys or recovery data belongs in this
branch. Preserve upstream LICENSE.md; new port sources use the same MIT terms.

## Earlier offline work on 2026-10-22

The catalog now derives systems from emulator definitions independently of games;
see [platform coverage](PLATFORMS.md). The local package provides 35 profiles and
32 available sections, including empty systems. Xbox font atlases now use linear
minification/magnification to retain grayscale antialiasing when text is scaled.
Font sizes, layout and other platforms are unchanged. A PC sampling comparison
illustrates the effect; TV legibility and system-logo artwork need separate
visual acceptance. Seven host test groups passed at that checkpoint, when the
candidate was still offline. Later deployment and TV acceptance are recorded above.

## Resumed console checks on 2026-10-22

The user subsequently allowed console work. A read-only inspection captured all
76 runtime options. The no-game arm/restore probe exposed a plugin-status marker
transition; the comparator now accepts only the documented loaded-to-found
transition while preserving all configuration fields. Regression tests cover it.
The corrected probe passed, restored all options and left saved boot files
unchanged. It was disabled afterward. The updated app and Tetris-only settings
passed full FTP readback. The later user-operated game/Guide-return check passed,
but restoration after that gameplay did not; a separate menu exit without a game
restored all 76 options exactly. No game was remotely launched. See
[session return](SESSION_RETURN.md) and
[console evidence](tests/hardware-session-return-2026-10-22.json). The later TV
feedback and complete catalog-build return test are recorded below.

## Offline follow-up and separate installed-build checks

The next candidate retains 34 available Salvia sources in 32 sections, including
both Arcade and Neo Geo Pocket backends. It routes each game by its physical
source directory, keeps separate metadata for grouped sections, and preserves
saved selections across equivalent path casing/drive aliases. Scraper IDs are
validated independently of theme names. See [platform routing](PLATFORMS.md).

Restoration rechecks runtime state after staging its private file; failed
handoffs show recovery errors in the UI. Nine host test groups and the full
Xbox cross-build pass. The package helper also passes four offline tests with
fake FTP, including duplicate filenames and source-specific artwork. This
candidate was not deployed at that offline checkpoint. The earlier Tetris build's
game exit and selected-game restoration are user-confirmed. Return to Aurora opens
Aurora, but its post-game runtime restoration did not pass inspection. A later
exit-only check passed exact restoration of all 76 options and logged a clean
exit from the expected HDD app. The earlier post-game mismatch remains
unexplained; the later catalog-build result is recorded separately below.

The subsequent offline check adds the optional production-model test above.
All ten enabled host test groups pass, including independent scan-only and
metadata-only phases. Salvia preflight now rejects a missing predecessor instead
of selecting the wrong positional system, and diagnostic exits use the same
session restoration as manual Quit. The full native build passes. These changes
were tested separately from the earlier manual-test build; see
[offline evidence](tests/offline-context-2026-10-22.json).

## Grouped catalog browser-only console check (2026-10-22)

Candidate source `ea0482ef7452f9894cac032140c105abe96ec317`, XEX SHA-256
`3e3d7dd490bb09408ee6a8a5af5f03f886006eb4c81164ebd5c40a751761209a`,
was deployed with 104 read-back-verified files: 3 replacements and 101 additions.
Eight protected files stayed unchanged. The 32 configured sections cover 34
sources; empty sections are currently omitted from the carousel. The five
populated lists contain Arcade 17, Game Boy 5, Nintendo 64 1, NES 10 and SNES 11
entries. All 43 original entries retained their metadata; the extra Arcade entry
was found in an existing second backend's ROM folder.

With every game launch blocked, the 1,200-frame diagnostic loaded and visited
all five populated lists, saved six native captures and exited cleanly to Aurora.
The original application settings were then restored byte-for-byte; selection
and saved boot settings were unchanged. Native XML rewrites changed formatting,
but the original metadata fields were preserved. This verifies catalog loading,
scanning and browser execution, not the newly installed build's emulator handoff
or restoration after gameplay.

Apparent missing text in image previews was traced to the preview display:
the unchanged PNG pixels contain the text, and reopening previews shows it.
No renderer change is justified by that artifact. The subsequent TV feedback and
gameplay check are distinct from this browser-only diagnostic. The earlier
post-game runtime mismatch and successful exit-only restoration remain separate
historical results. See
[sanitized catalog evidence](tests/hardware-catalog-2026-10-22.json).

## Catalog-build Tetris return and cleanup (2026-10-22)

The same catalog build and installed executable hash above passed a full manual
sequence: ES -> NES Tetris plays -> Xbox Guide/Y -> ES with Tetris selected ->
Start -> Quit -> Return to Aurora -> Yes. Read-only XBDM confirmed ES after the
game and Aurora after the menu exit, before the inspection probe ran.

Logs archived before any ES relaunch show restored game-list selection and clean
shutdown; the SDK log identifies the expected HDD image and exit status 0 to
Aurora. The executable readback matches the catalog candidate hash. The original
snapshot matches the known restored baseline, and the inspect-only probe found
all 76 runtime options byte-identical to it. Production exact and accepted-reload
comparisons both passed, with no changed option names; liveblock, livestrong and
noupdater remained enabled. Saved boot settings were unchanged and the probe
was disabled afterward. The earlier build's post-game mismatch remains
unexplained; this is a separate successful run. Other cores, repeated-session
stability and disc/native-game support still require their own checks.
See [sanitized game-return evidence](tests/hardware-tetris-return-catalog-2026-10-22.json).

On this game-tested build, the user reported good game names and ES menu text,
but fuzzy grey labels and unsmoothed system logos. The later visual build
`1222c78c167d` received TV acceptance: both labels and logos look better. That
visual result does not establish gameplay or return behavior on the newer binary.
