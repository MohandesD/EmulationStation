# Release validation

The release executable was built in a new native output directory using the
pinned open-source dependencies and the `EIGEN_MPL2_ONLY` compile guard.
It is a different binary from the one on the working console. No console was
contacted, no file deployed, and no startup/security/controller setting changed
while preparing the distribution.

Release checks operate on an independently extracted ZIP, not the user's app
folder. The exact machine-readable result is published beside the download as
`validation.json`, identifying the archive checksum and source commit.

The checks cover file/checksum completeness, FATX component lengths, XEX header,
safe disabled defaults, external configurations/theme includes, source/license
files, stock Salvia catalog regeneration, missing-core/menu/config rejection,
and production configuration/route/launch/theme parsing with synthetic text
fixtures. The fixtures are created outside the package, are not games, are never
executed and are not included in the downloadable app.

Previous console acceptance is documented in the source's
`platform/xbox360/ACCEPTANCE.md`. It covers the previous visual build on one
Trinity/17559 setup, with six user-tested games and independently verified final
return cleanup. It does not prove this package's clean-install TV appearance,
every game/platform, all multiplayer controls, or automatic startup. The EBA
X-Men control issue (special attack on several buttons) remains deferred and no
mapping is supplied or changed here.

A future new installation needs one grouped manual session: browse, play one
game per intended core, Guide/Y back to ES, then ES's Return to Aurora. This
release preparation does not require interrupting or replacing the working
family console to repeat already accepted tests.
