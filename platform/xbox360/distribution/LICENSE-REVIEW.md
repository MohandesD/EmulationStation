# Distribution notices and source

Reviewed for this preview on 2026-10-22. The application is built using the
open-source xsdk/GCC toolchain; no Microsoft XDK library, installer, header bundle,
debug-monitor plugin or proprietary SDK component is redistributed. Salvia and
its separately licensed cores remain separate downloads. No ROM, BIOS, game
artwork, music, saved console state or account credential is included.

The port and original Clean theme use the EmulationStation MIT license. This
does not replace the component licenses listed below. Their complete notices
are in the app's `licenses` folder and must accompany copies of the app.

| Component | License/choice and supplied notice |
| --- | --- |
| EmulationStation, including upstream UI resources | MIT; EmulationStation-MIT.txt |
| Original Clean theme XML | MIT; theme LICENSE.txt plus its copyright header |
| Embedded Open Sans font | Apache-2.0; OpenSans-notice.txt; full Apache text in MbedTLS-license.txt |
| xsdk runtime | MIT, with BSD-3-Clause libxenon/Xenia portions; xsdk-MIT.txt and xsdk-third-party.txt |
| SDL-xenon / SDL3 | zlib; SDL-zlib.txt and additional notices |
| FreeType | FreeType License selected, not its alternative GPL; FreeType-FTL.txt and contributed-file notices |
| Eigen headers | MPL-2.0 and permissive portions; MPL-2.0.txt, Eigen-BSD.txt, Eigen-MINPACK.txt; exact source included |
| Boost headers | Boost Software License 1.0; Boost-1.0.txt |
| stb image/resize | MIT option selected; stb-dual.txt retains both upstream options |
| pugixml | MIT; pugixml-MIT.txt |
| NanoSVG | zlib; nanosvg-zlib.txt |
| curl-xenon | curl license and included third-party notices; curl-license.txt and Additional-notices.txt |
| Mbed TLS / Project Everest / p256-m | Apache-2.0 option selected; MbedTLS-license.txt and additional attribution |
| libgcc / libstdc++ | GPL-3.0 with GCC Runtime Library Exception 3.1; GCC-GPL3.txt and GCC-runtime-exception.txt |
| newlib C runtime | Its collected permissive notices; newlib-notices.txt |
| Mozilla CA bundle | MPL-2.0; MPL-2.0.txt; original PEM header retained |

This software uses the FreeType project. Portions are copyright the FreeType
Project; all rights reserved. This product includes software developed by the
University of California, Berkeley and its contributors. These acknowledgments
do not imply endorsement.

`EIGEN_MPL2_ONLY` is enabled for the native build: Eigen rejects inclusion of its
LGPL-only modules. Mbed TLS's Apache option and FreeType's FTL option are selected.
Compiling with GCC is separate from redistributing GCC; the package ships neither
compiler nor toolchain. The linked GCC runtimes use the documented exception.
The complete exception and newlib notices are retained instead of treating all
linked code as the application's MIT license.

## Corresponding source and changes

- Application and package sources: `source/EmulationStation-source.zip` at the
  root of the download; also [the exact source commit](https://github.com/MohandesD/EmulationStation/tree/@SOURCE_COMMIT@).
  Xbox changes are marked in that fork. The executable adds the Eigen compile
  guard to the previously tested runtime source; there is no emulator binary patch.
- Eigen's exact unchanged headers, with all included notices, are in
  `source/Eigen-source.zip`. The source remains under its original licenses,
  including MPL-2.0; the program's MIT license does not limit those rights.
- `source/SOURCES.json` records exact upstream revisions and repository URLs for
  xsdk, SDL, FreeType, Eigen, stb, curl and Mbed TLS. The source build guide and
  pinned toolchain versions are in the included application source archive.
- `cacert.pem` is unchanged from the pinned Mbed TLS checkout's
  `programs/xsdk/cacert.pem`, a text/source-form PEM dated 2026-09-25. Its
  [pinned file](https://github.com/xenosworks/mbedtls-xenon/blob/591b04adbec4bc5582649601f4407506a610162b/programs/xsdk/cacert.pem)
  remains available as source under MPL-2.0. See
  [curl's CA extraction/license documentation](https://curl.se/docs/caextract.html).
  It is a public trust store, not a private key, and is not claimed current for
  every future release. No network access is necessary for local gameplay.

The original Simple theme is not bundled. It has separate CC-BY-NC-SA terms and
console/logo assets; the package instead includes original text-only theme XML.
Users may install other themes separately with their licenses. System names in
configuration identify supported platforms and do not imply endorsement.

The review covers this explicit package allowlist and pinned dependencies. It
does not grant permission to redistribute Salvia's entire archive, add other
assets, or include another user's saves, logs, ROMs or BIOS files.
