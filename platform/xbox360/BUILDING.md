# Building the Xbox 360 port

These instructions build the native application with the open-source xsdk and
GCC toolchain. Microsoft's XDK is not used. Run commands in a Linux shell; the
tested host is Debian 13 under WSL 2. See [installation](INSTALL.md) for the
runnable folder and [current results](README.md#current-state-2026-10-22) for
the limits of console testing.

## Host tools and working directories

Install the SDK's host prerequisites plus the shader compiler and Boost headers:

```sh
sudo apt update
sudo apt install build-essential bison flex texinfo libgmp-dev libmpfr-dev \
  libmpc-dev curl xz-utils patch python3 cmake ninja-build git glslang-tools \
  libboost1.83-dev
```

Use CMake 3.25-3.31 (tested 3.31.6) and Python 3.11 or newer. The inherited
top-level project still declares legacy CMake policies; these instructions do
not establish CMake 4 compatibility. The dependency preparation
script requires Boost 1.83 headers (tested package `1.83.0-4.2`); a different
distribution's default Boost may not match. Only the Boost headers are copied
into the dependency directory. Do not add `/usr/include` to the cross-compiler's
include path, which would mix host libc headers into Xbox builds.

From the root of this EmulationStation checkout, set the following variables in
the same shell. Use a new working directory for a new build environment:

```sh
XBOX_WORK="$HOME/xbox-build"
SDK="$XBOX_WORK/es-xsdk"
TOOLCHAIN="$HOME/xsdk-es-gcc"
DEPS="$XBOX_WORK/es-deps"
BUILD="$XBOX_WORK/es-app"
HOST_BUILD="$XBOX_WORK/es-host-tests"
mkdir -p "$XBOX_WORK"
```

## Pinned SDK and full C/C++ compiler

The SDK and source dependencies are pinned in [dependencies.json](dependencies.json).
For a new SDK checkout:

```sh
git clone https://github.com/xenosworks/xsdk.git "$SDK"
git -C "$SDK" checkout --detach 57b02f1e60667e37d22c3aa613d23d2486514ff8
PREFIX="$TOOLCHAIN" WORK="$XBOX_WORK/es-toolchain-work" JOBS=4 \
  bash "$SDK/tools/toolchain/build.sh"
```

For an existing environment, verify its SDK revision and local changes before
reuse; do not reset an unrelated checkout or replace an existing compiler.
The pinned script builds patched `powerpc-xsdk-elf` GCC 15.3.0 for **both C and
C++**, binutils 2.47 and newlib 4.6.0.20260123, and runs its ABI checks. A C-only
compiler from a graphics experiment cannot build the full application.
The SDK's [pinned getting-started guide](https://github.com/xenosworks/xsdk/blob/57b02f1e60667e37d22c3aa613d23d2486514ff8/docs/GETTING_STARTED.md)
provides more toolchain detail.

Build the SDK library and host shader compiler with that same toolchain:

```sh
make -C "$SDK" -j4 TOOLCHAIN="$TOOLCHAIN/bin/powerpc-xsdk-elf-" \
  build/libxsdk.a build/host/libxsc.so
```

The SDK Makefile takes an **executable prefix** ending in `powerpc-xsdk-elf-`.
CMake below takes the compiler's **installation directory** instead. Omitting
the Makefile override can silently select a different compiler at its default
location. Use a separate SDK build/checkout if changing toolchains.

## Dependencies and native application

```sh
python3 platform/xbox360/prepare-deps.py "$DEPS" --boost-include /usr/include
cmake -S . -B "$BUILD" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$SDK/cmake/xsdk.cmake" \
  -DXSDK_TOOLCHAIN="$TOOLCHAIN" -DES_DEPS="$DEPS" \
  -DES_GLSLANG=/usr/bin/glslangValidator -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j4
```

`prepare-deps.py` clones the pinned SDL, FreeType, Eigen, stb, curl and mbedTLS
sources, initializes required submodules, and isolates Boost's headers. It
refuses to reset an existing checkout at another revision or use tracked local
changes. It does not download emulators, games, BIOS or themes.

The runtime outputs are:

- `$BUILD/es-app/emulationstation.xex`
- `$BUILD/cacert.pem` (the HTTPS CA bundle, read as `game:/cacert.pem`)

ELF files and generated shader headers are build/debug artifacts and are not
needed in the installed app folder. See [INSTALL.md](INSTALL.md) for configuration
and separately licensed theme assets. The existing native build has passed;
the documented CMake command was also checked in a fresh build directory on
2026-10-22. That fresh-directory check configured/generated successfully; it was
not a second full compilation or a console test.

## PC tests and optional diagnostics

Use a separate **host** build directory, without the Xbox toolchain file:

```sh
cmake -S platform/xbox360/tests -B "$HOST_BUILD" -G Ninja \
  -DES_STB="$DEPS/stb" -DES_MODEL_DEPS="$DEPS"
cmake --build "$HOST_BUILD" -j4
ctest --test-dir "$HOST_BUILD" --output-on-failure
```

`ES_MODEL_DEPS` enables the production configuration/game-list model tests; with
it, ten test groups are configured. Native host tests use address and undefined
behavior sanitizers. These validate software behavior with fixtures, not emulator
gameplay, console input or runtime return settings.

Optional graphics and integration probes can be built separately:

```sh
make -C platform/xbox360 -f Makefile.graphics SDK="$SDK" \
  TC="$TOOLCHAIN/bin/powerpc-xsdk-elf-" BUILD="$XBOX_WORK/es-graphics" \
  GLSLANG=/usr/bin/glslangValidator
make -C platform/xbox360 -f Makefile.probes SDK="$SDK" \
  TC="$TOOLCHAIN/bin/powerpc-xsdk-elf-" BUILD="$XBOX_WORK/es-probes"
```

They are not required app files. Read each probe's source and documented mode
before console use: some change ordinary emulator menu state or temporary return
settings. The Xenia instruction scanner reports instructions it cannot model in
the current application/dependencies; Xenia is not a substitute for console
acceptance. See [the evidence and probe notes](README.md).
