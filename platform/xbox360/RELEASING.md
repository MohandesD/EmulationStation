# Building and checking an Xbox download

`build-release.py` is an offline allowlist packager, not an uploader. Build the
native application using BUILDING.md first, including the Eigen MPL2 compile
guard. Supply exact pinned source checkouts and an unmodified Salvia v1.0.10 ZIP
only for inventory metadata. No Salvia files enter the result.

```sh
python3 platform/xbox360/build-release.py \
  --source "$PWD" --build "$BUILD" --deps "$DEPS" --sdk "$SDK" \
  --toolchain-work "$XBOX_WORK/es-toolchain-work" \
  --salvia-zip /path/to/Salvia-360.zip \
  --boost-license /path/to/boost-1.83-LICENSE_1_0.txt \
  --output /path/to/new-release-directory
```

The Boost notice is the official Boost 1.83 LICENSE_1_0.txt. Commit the reviewed
source locally first; the packager refuses a dirty checkout, modified pinned
dependencies, a mismatched Salvia digest or an existing output directory. It
retains the XEX, CA bundle, generated catalog, original theme, external settings,
notices and corresponding application/Eigen source archives. Nothing is copied
from a live console or an existing user's app folder.

Build the host tests with ES_MODEL_DEPS as in BUILDING.md, then validate the ZIP:

```sh
ctest --test-dir "$HOST_BUILD" --output-on-failure
python3 platform/xbox360/tests/test_distribution.py \
  --archive /path/to/EmulationStation-Xbox360-0.1.0-preview.1.zip \
  --salvia-zip /path/to/Salvia-360.zip \
  --model "$HOST_BUILD/test_es_distribution" \
  --report /path/to/validation.json
```

The test extracts into a fresh temporary directory and executes only PC
verification/configuration scripts and the production host model. It creates
inert text fixtures outside the package. The XEX, cores and games are never run.
The production theme parser resolves the packaged include paths. Resource
existence for the known embedded font is a host seam, not a renderer test.

Publish only after approval and successful checks. Check for an existing tag and
release before creation, and inspect uncertain outcomes before retrying. Release
assets are the application ZIP, SHA256SUMS.txt, validation.json and accurate notes.
Download each uploaded asset through its public URL and compare bytes/checksums;
validate the downloaded ZIP again. Never claim the new package was console-tested
unless an actual new-installation test occurred. Keep existing console settings,
user configurations separate from publication.
