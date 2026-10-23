# Working Xbox port acceptance — 2026-10-22

Objective: build a working Xbox 360 version of EmulationStation and publish an
upstream PR or a public fork. The implementation is the original EmulationStation
application, based on upstream `9cc42adff67946175d2b7e25c6ae69cc374e98a0`, with a
native Xenos renderer and open-source xsdk/SDL adapters. It is not the earlier
custom dashboard preview.

The working development port and public fork satisfy that objective with the
evidence below. This is acceptance of the implemented frontend and tested setup,
not a claim of universal game compatibility or a production replacement dashboard.

| Requirement | Evidence and scope |
| --- | --- |
| Native Xbox application builds and runs | The full C/C++ build produced `emulationstation.xex`; the local output and post-batch console readback both hash to `1222c78c167d60dedde6809f71d8298d4b4fc047727c0c1e5f7fc0b97eb4408c`. Runtime source is `9a15e37cfda015339cf3a8b5887abd533d47ea27`; later commits through the batch report only change documentation/evidence. [Build procedure](BUILDING.md), [build/visual evidence](tests/image-readability-candidate-2026-10-22.json). |
| Controller-driven system/game browsing with artwork | User confirmed system selection, game lists, Back, menu exit and saved system selection. Native captures verified five populated lists and covers. Later user feedback accepted game names, menu text, grey-label readability and system-logo smoothing. [Controller evidence](tests/hardware-controller-2026-10-22.json), [catalog evidence](tests/hardware-catalog-2026-10-22.json), [visual evidence](tests/image-readability-candidate-2026-10-22.json). |
| Launch configured emulators/games | All 44 current commands passed production parser/Salvia-context preflight against archived configuration. The user then reported all six representative games worked: Tetris, Super Mario World, Kirby's Dream Land, X-Men through FBNeo, X-Men through MAME and Kirby 64. MAME X-Men's control issue is deferred below. [Batch setup](tests/manual-library-batch-2026-10-22.json), [gameplay report](tests/hardware-library-batch-2026-10-22.json). |
| Return from a game to ES and leave ES for Aurora | The detailed NES test confirmed Guide/Y returns to ES with Tetris selected, then the ES menu returns to Aurora. The later batch report did not individually describe each exit action. Its final Aurora state, clean ES shutdown and saved Kirby 64 selection were independently observed. [Detailed return test](tests/hardware-tetris-return-catalog-2026-10-22.json), [batch cleanup](tests/hardware-library-batch-2026-10-22.json). |
| Temporary return configuration is restored | After the current batch, the inspect-only probe read all 76 runtime options. Production exact/reload-equivalent comparisons matched the original baseline, with zero changed names; network/update options remained enabled and saved boot settings were unchanged. The probe was disabled afterward. [Return design](SESSION_RETURN.md), [inspection evidence](tests/hardware-library-batch-2026-10-22.json). |
| Platform coverage is independent of the current collection | The local Salvia inventory yields 35 profiles, 34 available sources and 32 sections. Empty systems are configured and hidden until populated. Grouped Arcade/NGP sources retain their own core/context and metadata. One profile with a missing binary is recorded unavailable. This is catalog/integration coverage, not hardware testing of every platform. [Platform catalog](PLATFORMS.md), [production-model tests](tests/offline-context-2026-10-22.json). |
| Publish the implementation with build/install guidance | GitHub confirms [MohandesD/EmulationStation](https://github.com/MohandesD/EmulationStation/tree/xbox360) is a public fork of Aloshi/EmulationStation. Its `xbox360` branch contains source, tests, pinned dependencies, [build instructions](BUILDING.md), [installation/removal instructions](INSTALL.md), license and scoped evidence. Creating this fork satisfies the publication alternative; no upstream PR is claimed. |

The ten-group host suite passed for the catalog/return implementation. The only
subsequent runtime source change was non-tiled image magnification filtering;
the three relevant graphics/image checks and four offline package checks passed
for that visual update. The final audit inspected those records and matched the
built/installed executable instead of claiming a new full test run. Host tests
and Xenia do not substitute for the cited console/user observations.

## Deferred and future work

- **EBA X-Men controls:** the user deferred this issue through the coordinating
  chat. Its existing game-specific mapping is unchanged. Specific incorrect
  buttons have not been described, and no correction or successful mapping test
  is claimed. This deferred emulator configuration issue does not block acceptance
  of the working port.
- **Broader compatibility:** additional cores/games, four simultaneous players,
  sustained sessions, scraping/network features and other console environments
  have not all been tested. Future results should identify the tested build and
  configuration rather than extending the current report to every platform.
- **Automatic startup and native Xbox games:** persistent startup, disc/HDD Xbox
  360 game flows and Steam streaming are separate project follow-ups. This work
  did not install a persistent modification, flash NAND, format a drive or change
  the saved startup configuration. The current setup remains a temporary session
  with manual frontend launch.

No additional hands-on step is required to establish this working-port result.
Future compatibility or startup work needs its own concrete scope and, where
applicable, installation/recovery decision. Raw console records, ROMs, BIOS,
recovery data and licensed theme assets remain outside the source repository.
