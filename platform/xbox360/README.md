# EmulationStation for Xbox 360 — port in progress

This branch ports **Aloshi/EmulationStation itself**, based on
`9cc42adff67946175d2b7e25c6ae69cc374e98a0`. The final target is a native `.xex`
running the existing system carousel, game lists, themes, artwork and controller
navigation, with emulator launch and a verified way back to the frontend.
The earlier custom Games dashboard is a separate project, not this port.

## Current state (2026-10-22)

The graphics component cross-compiles into `ESGraphicsProbe.xex`. This diagnostic
is **not a working EmulationStation build** and has not run on the console yet.
The complete application's Xbox build, input/audio adaptation, dependencies,
launch/return integration and on-console acceptance remain unfinished.

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

`Renderer_init_xsdk.cpp` supplies the renderer lifecycle, but is **not yet wired
into a complete application build target**. The next change must integrate the
real es-core/es-app targets, rather than replace them with another dashboard.

## Reproducible component checks

SDK: https://github.com/xenosworks/xsdk at
`57b02f1e60667e37d22c3aa613d23d2486514ff8` (MIT and included third-party notices).
Compiler: patched powerpc-xsdk-elf GCC 15.3.0, newlib 4.6.0.20260123,
binutils 2.47, built with that revision's `tools/toolchain/build.sh`.
The SDK's patches match the already working C-only compiler from the earlier
dashboard. A separate full C/C++ toolchain is being built; do not replace or
delete the installed C-only compiler.

On a Linux/WSL host, from the repository root:

```sh
cmake -S platform/xbox360/tests -B build-es-tests -G Ninja
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

Host tests and successful XEX packaging do not establish console rendering.
The SDK's instruction scanner reports three instructions in linked SDK/newlib
functions that Xenia skips; Xenia is not a substitute for a console run.

## Next integration work

1. Finish/revalidate the separately running C++ compiler build; compile and run
   a C++/filesystem/exception diagnostic using the pinned SDK before relying on it.
2. Use SDL-xenon (`xenon` branch, pin a commit) for controllers, audio and timing.
   Its documentation explicitly has no native video renderer; use this Xenos
   adapter. Adapt SDL2 interfaces to SDL3 rather than assume ABI compatibility.
3. Cross-build FreeType and required image/network dependencies. Keep Eigen and
   date-time headers; use the SDK-supported filesystem API or port Boost's.
4. Integrate existing es-core/es-app sources, theme/resource loading, FATX paths,
   controller defaults and an ordinary HDD application package.
5. Test real carousel, game lists, covers, transitions, input and memory lifetime
   on the user's Trinity/17559 temporary XeUnshackle session.
6. Add native XEX and Salvia handoff with documented launch data. The earlier
   Games direct-ROM handoff crashed: do not recycle it as a proven implementation.
   Prove emulator start and return without changing saved boot configuration.
7. Publish a maintained Xbox fork or an upstream PR, with a reproducible build,
   notices, installation/rollback instructions and explicit hardware results.

The component probe shows RGB blocks, a translucent overlay, a clipped rectangle
and a four-colour texture. B requests a return to HDD Aurora through xsdk's
documented launcher. This return path and the visuals remain unverified. No ROM
is launched by the probe. Do not set it as the default dashboard.

No ROM, BIOS, commercial artwork, console keys or recovery data belongs in this
branch. Preserve upstream LICENSE.md; new port sources use the same MIT terms.
