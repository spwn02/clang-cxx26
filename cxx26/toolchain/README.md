# clang-cxx26 reference toolchain

This directory defines the reproducible snapshot format used by Miracle, Switch, and Nyx.

It is intentionally separate from LLVM's upstream release machinery.

## Snapshot identities

Published reference snapshots use annotated Git tags:

```text
cxx26-YYYY.MM.DD
cxx26-YYYY.MM.DD.N
```

Examples:

```text
cxx26-2026.08.22
cxx26-2026.08.22.2
```

A published identifier is immutable. Never move an existing snapshot tag or replace its release assets. If the bytes or source revision change, publish a new identifier.

Non-publishing preflight artifacts use:

```text
cxx26-dev-<commit>
```

They are triggered by disposable tags named:

```text
cxx26-preflight-<name>
```

The trigger tag name is not the artifact identity. A preflight always packages the tagged commit as `cxx26-dev-<commit>`.

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

The installed Clang defaults to libc++ and lld on Linux. The packaged CMake toolchain file also makes those choices explicit and enables `-freflection-latest`, which is the current fork-specific switch for the complete reflection implementation used by the reference C++26 mode.

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
cxx26/toolchain/build-linux-x86_64.sh \
  /tmp/cxx26-stage \
  /tmp/cxx26-build
```

Then package a development snapshot:

```bash
python3 cxx26/toolchain/package.py \
  --stage /tmp/cxx26-stage \
  --output /tmp/cxx26-dist \
  --snapshot cxx26-dev-$(git rev-parse --short=12 HEAD) \
  --revision $(git rev-parse HEAD) \
  --source-date-epoch $(git show -s --format=%ct HEAD)
```

## Use an extracted snapshot

After extracting the archive:

```bash
source clang-<snapshot>/share/clang-cxx26/activate.sh
```

This defines:

```text
CXX26_TOOLCHAIN_ROOT
CXX26_CMAKE_TOOLCHAIN_FILE
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
  -DCMAKE_TOOLCHAIN_FILE="$CXX26_CMAKE_TOOLCHAIN_FILE"
```

The toolchain file is relocatable and derives all paths from its own installed location.

It also enables the current reference-fork reflection mode automatically. Consumer CMake projects should target standardized C++26 source semantics rather than repeat `-freflection-latest` themselves merely to select the reference implementation.

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
share/clang-cxx26/manifest.json
```

The manifest records the exact source commit, platform, compiler identity, libc++ module manifest, included components, and CMake integration entrypoint.

Miracle and Switch CI will later pin both the snapshot identifier and the expected archive digest. They must never follow the moving `cxx26` branch.

## Validation boundary

T3 performs a packaging smoke test after extracting the archive into a new location. The smoke test verifies that:

- the artifact is relocatable;
- the packaged Clang can be selected through the generated CMake toolchain file;
- CMake sees C++26 `import std`;
- the installed libc++ module sources are usable;
- a small C++26 program can build and execute.

Fine-grained language/library feature probing belongs to T4.

The T3 relocation smoke test nevertheless exercises a minimal reflection path
through `import std`, including libc++'s reflected type-trait wrappers. This is
intentional: the reference snapshot is not considered usable if its packaged
standard module cannot consume the reflection-enabled `<meta>` implementation.

## CI persistence and caching

The snapshot workflow separates expensive production from validation:

```text
exact staged-toolchain artifact?
      |
      +-- yes --> restore installed compiler/runtime stage
      |
      `-- no --> restore latest rolling ccache artifact
                    |
                    v
               build + install
                    |
                    +--> persist exact staged-toolchain artifact
                    |
                    `--> persist updated rolling ccache artifact
                              |
                              v
                           package
                              |
                              +--> candidate artifact
                              |
                              v
                  separate relocation/smoke verification
                              |
                              v
                  publish dated snapshots only
```

The packaged candidate is uploaded before verification begins. A failed smoke
test therefore does not discard the already-built snapshot.

GitHub Actions dependency caches are deliberately not used for the reusable
toolchain state. Cache visibility is scoped by branch/tag ref, so a cache
created by `cxx26-preflight-a` cannot be restored by
`cxx26-preflight-b`. Disposable preflight tags therefore make the ordinary
Actions cache service unsuitable for this workflow.

Instead, the workflow uses repository-level Actions artifacts as a
content-addressed build store:

```text
cxx26-stage-linux-x86_64-v2-<source-fingerprint>-<recipe-fingerprint>
cxx26-ccache-linux-x86_64-v2
```

The staged-toolchain artifact is exact. Its fingerprint is derived from the
compiler/runtime source trees plus the toolchain build recipe. Changes only to
packaging, activation, verification, documentation, or the generated CMake
toolchain integration reuse the installed stage without rebuilding LLVM.

The ccache artifact is intentionally *not* keyed by the source revision. ccache
performs its own content/dependency validation, so a source edit can reuse all
unaffected object compilations. This is what makes a change to one libc++ header
different from a cold compiler rebuild: the exact installed stage is rebuilt,
but unrelated compiler objects are served from the rolling compiler cache.

Artifacts are repository-level and can be located and downloaded across
different preflight/release tag runs, avoiding the tag-scope restriction of the
Actions cache service.

During migration from the original cache implementation, the workflow retains
a restore-only legacy `actions/cache` bridge. Reusing the same disposable
preflight tag name once lets an existing tag-scoped stage/ccache entry be
imported and republished as repository-level artifacts. New reusable state is
not written back to the tag-scoped cache service.

Exact staged-toolchain artifacts are retained for 30 days, rolling compiler
cache artifacts and candidate snapshots for 14 days. Published dated snapshots
remain the immutable long-term distribution mechanism.

## Publishing

The GitHub workflow `.github/workflows/reference-toolchain-snapshot.yml` has two modes.

A `cxx26-preflight-*` tag builds and validates a temporary preflight artifact only.
It does not create a GitHub release. Preflight tags may be deleted after the run.

For example:

```bash
git tag cxx26-preflight-t3
git push origin cxx26-preflight-t3
```

After the workflow has completed, the disposable trigger tag can be removed:

```bash
git push origin :refs/tags/cxx26-preflight-t3
git tag -d cxx26-preflight-t3
```

Pushing an annotated `cxx26-YYYY.MM.DD[.N]` tag builds the same artifact and creates a GitHub prerelease after validation. The workflow refuses lightweight tags, tags whose commit is not in `cxx26` history, and release identifiers that already have a GitHub release.

The workflow never creates or moves the source tag itself.
