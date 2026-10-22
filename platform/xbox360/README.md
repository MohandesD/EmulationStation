# EmulationStation for Xbox 360 — port in progress

This branch ports **Aloshi/EmulationStation itself**, based on
`9cc42adff67946175d2b7e25c6ae69cc374e98a0`. The final target is a native `.xex`
running the existing system carousel, game lists, themes, artwork and controller
navigation, with emulator launch and a verified way back to the frontend.
The earlier custom Games dashboard is a separate project, not this port.

## Current state (2026-10-22)

The graphics component cross-compiles into `ESGraphicsProbe.xex` and has run on
the Trinity/17559 console. A native 1280x720 capture shows correct RGB blocks,
alpha blending, scissors and texture colours; 14 pixel checks and a native line check pass. After 1,800
frames the diagnostic automatically returns to HDD Aurora, observed via XBDM.
The full C/C++ toolchain and real es-core/es-app now cross-build successfully.
The native application has loaded five system lists (43 existing games), rendered
the upstream Simple theme's system carousel at 1280x720 and returned cleanly to
Aurora after a bounded 600-frame test. Its own fenced capture was visually checked.
This is **not a finished game frontend yet**: controller navigation, audio,
emulator launch/return and publication remain unfinished. No controller was
connected during these display tests, so input is not verified.

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

Xbox commands have the form `xex "hdd:/path/app.xex" "hdd:/path/game.nes"`.
They are parsed as paths, never sent to a shell. Salvia ROM data uses its own
`Hdd:`/`Usb0:` mounts. `XboxLaunchEnabled` remains false during initial frontend
bring-up; it must be removed as a bring-up gate or deliberately enabled and
hardware-tested before claiming the port works as a game frontend. Returning
from an emulator to ES is not implemented or verified yet.

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

On a Linux/WSL host, from the repository root:

```sh
cmake -S platform/xbox360/tests -B build-es-tests -G Ninja -DES_STB="$DEPS/stb"
cmake --build build-es-tests
ctest --test-dir build-es-tests --output-on-failure
make -C "$SDK" -j4 build/libxsdk.a build/host/libxsc.so
make -C platform/xbox360 -f Makefile.graphics SDK="$SDK" TC="$TC" BUILD="$BUILD"
```

Set SDK to the pinned SDK checkout, TC to the compiler prefix ending in
`powerpc-xsdk-elf-`, and BUILD to a separate output directory. The Makefile's
defaults match the current local WSL environment. C17 builds use warnings as
errors. Host tests enable address/undefined-behaviour sanitizers and keep asserts
enabled in release builds. Tests check actual ES array formats, nonzero draw
offsets, unaligned/interleaved floats, projection/translation, alpha atlas padding
and subupdates, raw RGBA order, clipping state, texture lifetime and errors.
The source inventory fails if upstream adds an unimplemented GL entry point.
Compiled shader fetch records must match POSITION0/COLOR0/TEXCOORD0 and sampler
slot zero; a deliberately wrong sampler is rejected by the validation script.

Four host tests cover GL semantics/source coverage, Xbox launch path parsing,
and image orientation/alpha/resize/failure handling, with sanitizers enabled.
They complement the separate graphics-only console evidence.
The SDK's instruction scanner reports three skipped instructions in the small
graphics probe and 160 in the full application/dependencies. Xenia results in
those functions are unreliable; Xenia is not a substitute for a console run.

## Full application build (integration in progress)

`dependencies.json` pins the exact source commits. On the tested Debian 13 host,
install Boost 1.83 headers (`libboost1.83-dev` 1.83.0-4.2) and the usual build
tools, then prepare dependencies without resetting any existing checkout:

```sh
python3 platform/xbox360/prepare-deps.py "$DEPS" --boost-include /usr/include
cmake -S . -B "$BUILD" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$SDK/cmake/xsdk.cmake" \
  -DXSDK_TOOLCHAIN="$TOOLCHAIN" -DES_DEPS="$DEPS" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j4
```

Build the pinned SDK and its shader compiler first, using its documented
toolchain script. TOOLCHAIN is the compiler installation directory, not its
executable prefix. Shader compilation locates `glslangValidator` or `glslang`; override with `-DES_GLSLANG=/path/to/tool` if needed.
The resulting target is `es-app/emulationstation.xex`. These full-build commands
have produced a native XEX; it remains a development build pending the runtime
acceptance checks below. No ROMs,
commercial artwork or BIOS are fetched by the dependency script.

## Display bring-up and installation

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
The default is zero, which leaves the frontend open normally. Game launching
remains gated by `XboxLaunchEnabled=false`. Hotplugged Xbox pads receive defaults;
an unplugged controller no longer strands startup in the desktop setup wizard.
Physical navigation and the menu's Return to Aurora action still need testing.

Rollback: while Aurora is running, remove only the separately added app folder
if desired. Existing emulators, ROMs, assets and startup settings are independent.
Do not set this development build as the default dashboard.

## Next integration work

1. Verify controller input, audio, filesystem/locale behavior and text input;
   dummy SDL text input is only an adapter, not a verified Xbox keyboard UI.
2. Test game lists, covers, transitions, input and memory lifetime
   on the user's Trinity/17559 temporary XeUnshackle session.
3. Add native XEX and Salvia handoff with documented launch data. The earlier
   Games direct-ROM handoff crashed: do not recycle it as a proven implementation.
   Prove emulator start and return without changing saved boot configuration.
4. Publish a maintained Xbox fork or an upstream PR, with a reproducible build,
   notices, installation/rollback instructions and explicit hardware results.

The component probe shows RGB blocks, a translucent overlay, a clipped rectangle
and a four-colour texture. B requests an early return to HDD Aurora through
xsdk's documented launcher; an automatic return after 1,800 frames is verified.
The B input itself has not been tested. No ROM is launched by the probe.
Do not set it as the default dashboard.

No ROM, BIOS, commercial artwork, console keys or recovery data belongs in this
branch. Preserve upstream LICENSE.md; new port sources use the same MIT terms.
