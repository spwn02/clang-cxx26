# clang-p2996 reference toolchain

This directory defines the reproducible snapshot format used by Miracle, Switch, and Nyx.

It is intentionally separate from LLVM's upstream release machinery.

## Snapshot identities

Published reference snapshots use annotated Git tags:

```text
p2996-YYYY.MM.DD
p2996-YYYY.MM.DD.N
```

Examples:

```text
p2996-2026.08.22
p2996-2026.08.22.2
```

A published identifier is immutable. Never move an existing snapshot tag or replace its release assets. If the bytes or source revision change, publish a new identifier.

Non-publishing preflight artifacts use:

```text
p2996-dev-<commit>
```

They are triggered by disposable tags named:

```text
p2996-preflight-<name>
```

The trigger tag name is not the artifact identity. A preflight always packages the tagged commit as `p2996-dev-<commit>`.

## Initial platform

T3 validates and packages:

```text
Linux x86_64
```

This is the initial validated snapshot platform, not a statement that the fork is inherently Linux- or x86-only.

## Contents

A snapshot is one coherent compiler/runtime unit:

```text
clang / clang++
clang-scan-deps
lld
llvm-ar
libc++
libc++abi
libunwind
libc++ standard-module sources and libc++.modules.json
```

The compiler and runtimes are built from the same source revision. libc++, libc++abi, and libunwind are configured through `LLVM_ENABLE_RUNTIMES`, so they are built by the just-built compiler rather than by an unrelated host compiler.

The installed Clang defaults to libc++ and lld on Linux. The packaged CMake toolchain file also makes those choices explicit.

## Build locally

Prerequisites:

```text
CMake
Ninja
Python 3
GNU tar
zstd
```

From the repository root:

```bash
p2996/toolchain/build-linux-x86_64.sh \
  /tmp/p2996-stage \
  /tmp/p2996-build
```

Then package a development snapshot:

```bash
python3 p2996/toolchain/package.py \
  --stage /tmp/p2996-stage \
  --output /tmp/p2996-dist \
  --snapshot p2996-dev-$(git rev-parse --short=12 HEAD) \
  --revision $(git rev-parse HEAD) \
  --source-date-epoch $(git show -s --format=%ct HEAD)
```

## Use an extracted snapshot

After extracting the archive:

```bash
source clang-<snapshot>/share/clang-p2996/activate.sh
```

This defines:

```text
P2996_TOOLCHAIN_ROOT
P2996_CMAKE_TOOLCHAIN_FILE
CC
CXX
PATH
LD_LIBRARY_PATH
```

For CMake:

```bash
cmake \
  -S . \
  -B build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$P2996_CMAKE_TOOLCHAIN_FILE"
```

The toolchain file is relocatable and derives all paths from its own installed location.

## Package metadata

Each release contains:

```text
clang-<snapshot>-linux-x86_64.tar.zst
clang-<snapshot>-linux-x86_64.manifest.json
clang-<snapshot>-linux-x86_64.SHA256SUMS
clang-<snapshot>-linux-x86_64.SHA512SUMS
```

The same manifest is also stored inside the archive at:

```text
share/clang-p2996/manifest.json
```

The manifest records the exact source commit, platform, compiler identity, libc++ module manifest, included components, and CMake integration entrypoint.

Miracle and Switch CI will later pin both the snapshot identifier and the expected archive digest. They must never follow the moving `p2996` branch.

## Validation boundary

T3 performs a packaging smoke test after extracting the archive into a new location. The smoke test verifies that:

- the artifact is relocatable;
- the packaged Clang can be selected through the generated CMake toolchain file;
- CMake sees C++26 `import std`;
- the installed libc++ module sources are usable;
- a small C++26 program can build and execute.

Fine-grained language/library feature probing belongs to T4.

## Publishing

The GitHub workflow `.github/workflows/reference-toolchain-snapshot.yml` has two modes.

A `p2996-preflight-*` tag builds and validates a temporary preflight artifact only.
It does not create a GitHub release. Preflight tags may be deleted after the run.

For example:

```bash
git tag p2996-preflight-t3
git push origin p2996-preflight-t3
```

After the workflow has started, the disposable trigger tag can be removed:

```bash
git push origin :refs/tags/p2996-preflight-t3
git tag -d p2996-preflight-t3
```

Pushing an annotated `p2996-YYYY.MM.DD[.N]` tag builds the same artifact and creates a GitHub prerelease after validation. The workflow refuses lightweight tags, tags whose commit is not in `p2996` history, and release identifiers that already have a GitHub release.

The workflow never creates or moves the source tag itself.
