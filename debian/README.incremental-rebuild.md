# Incremental kernel rebuilds

This tree provides developer targets for iterating on kernel patches without
discarding the existing object tree between builds.

## Why use this

Normal clean builds are useful for release validation, but they are slow for
patch-test loops because they remove `debian/build/build-<flavour>` and force
Kbuild to rebuild everything. The `rebuild-<flavour>` target preserves that
object tree, invalidates only the packaging/build stamps for the selected
flavour, and lets Kbuild rebuild only what changed.

This is intended for local developer testing. Use normal clean builds for final
validation.

## Targets

| Target | Purpose |
| --- | --- |
| `binary` | Full Debian packaging target: arch packages, indep packages, common headers/tools/source packages, and all enabled flavours. |
| `binary-generic` | Build and package only the `generic` flavour. This still runs the full generic install/package path. |
| `rebuild-generic` | Rebuild and repackage the `generic` flavour while preserving `debian/build/build-generic`. |
| `rebuild-<flavour>` | Same as above for another flavour, for example `rebuild-generic-64k`. |

## Basic usage

Start with an initial flavour build:

```bash
DEB_BUILD_OPTIONS=parallel=$(nproc) fakeroot debian/rules binary-generic
```

After changing kernel source, run:

```bash
DEB_BUILD_OPTIONS=parallel=$(nproc) fakeroot debian/rules rebuild-generic
```

The rebuild target removes:

```text
debian/stamps/stamp-build-generic
debian/stamps/stamp-install-generic
debian/build/abi-generic
```

It preserves:

```text
debian/build/build-generic
```

That preserved directory contains the existing `.config`, object files, and
Kbuild state used for incremental rebuilds.

## Rebuild package snapshots

After a successful `rebuild-<flavour>`, generated binary packages are copied to:

```text
../rebuilds/<DirectoryID>/
```

The package names are unchanged, so this does not require custom versioning or
extra rebuild work. The directory ID is derived from Git state:

| Tree state | Directory ID |
| --- | --- |
| clean committed tree | `<HEAD-short>` |
| dirty tracked changes | `<HEAD-short>-dirty-<diff-hash>` |

For example:

```text
../rebuilds/2d1fdcea6aef-dirty-1a2b3c4d5e6f/
  linux-image-unsigned-7.0.0-28-generic_7.0.0-28.28_amd64.deb
  linux-modules-7.0.0-28-generic_7.0.0-28.28_amd64.deb
  manifest.txt
```

The snapshot step can be disabled or redirected:

```bash
fakeroot debian/rules rebuild-generic do_rebuild_snapshot=false
fakeroot debian/rules rebuild-generic rebuild_snapshot_dir=/tmp/rebuilds
```

## Optional no-BTF developer mode

For faster local iteration, the initial build can disable BTF generation:

```bash
DEB_BUILD_OPTIONS=parallel=$(nproc) fakeroot debian/rules binary-generic do_skip_btf=true
```

This changes the generated build config for that object tree:

```text
CONFIG_DEBUG_INFO_BTF=n
CONFIG_DEBUG_INFO_BTF_MODULES=n
```

It also uses the existing `bpftool` stub path so packaging does not try to dump
`vmlinux.h` from a no-BTF `vmlinux`.

Subsequent rebuilds inherit the no-BTF config automatically because
`rebuild-generic` preserves `debian/build/build-generic/.config`:

```bash
DEB_BUILD_OPTIONS=parallel=$(nproc) fakeroot debian/rules rebuild-generic
```

If you run `debian/rules clean`, pass `do_skip_btf=true` again on the next
initial build. This mode skips the annotations config check because the local
developer config intentionally differs from Ubuntu policy.

## Validation test

The test script exercises the incremental workflow and saves logs/metrics for
later optimization work:

```bash
DEB_BUILD_OPTIONS=parallel=$(nproc) debian/tests/incremental-rebuild \
  --clean \
  --flavour generic \
  --log-dir ../incremental-rebuild-generic
```

No-BTF test run:

```bash
DEB_BUILD_OPTIONS=parallel=$(nproc) debian/tests/incremental-rebuild \
  --clean \
  --skip-btf \
  --flavour generic \
  --log-dir ../incremental-rebuild-generic-skip-btf
```

The test:

1. Optionally runs `debian/rules clean`.
2. Builds a baseline `binary-<flavour>`.
3. Temporarily patches `init/version.c` to exercise the image/vmlinux path.
4. Runs `rebuild-<flavour>` and verifies newer image/modules packages.
5. Temporarily patches `drivers/net/dummy.c` to exercise a module path.
6. Runs `rebuild-<flavour>` again and verifies newer packages.
7. Restores the modified source files on exit.

Saved artifacts include:

```text
full.log
metrics.tsv
image-packages.txt
module-packages.txt
*-packages.before.tsv
*-packages.after.tsv
```

## Measured results

On this system with `DEB_BUILD_OPTIONS=parallel=16`, the latest full rerun
measured:

| Scenario | With BTF | Without BTF | Saving |
| --- | ---: | ---: | ---: |
| Initial clean `binary-generic` | 48m25s | 39m03s | 9m22s |
| `init/version.c` / zImage rebuild | 11m46s | 7m48s | 3m58s |
| `drivers/net/dummy.c` / module rebuild | 8m16s | 7m38s | 38s |

The no-BTF run produced zero BTF-related build lines, while the BTF-enabled run
produced 13,810 BTF-related lines across the test.

## Remaining bottlenecks

Even with BTF disabled, module rebuilds still spend time in global module
finalization and packaging. A module edit still triggers `MODPOST
Module.symvers`, then `modules_install`, strip, signing, and package generation
for thousands of modules.

Potential future optimizations:

1. Add a compile-only target that stops after `stamp-build-<flavour>`.
2. Add a developer-only module repack target that replaces only changed modules
   in an existing staging tree and rebuilds only `linux-modules-*`.
3. Add finer-grained timing markers around Kbuild, `modules_install`,
   module-signature checks, ABI generation, and each package build step.
