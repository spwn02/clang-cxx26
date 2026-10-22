# cxx26/dev: verification tooling of the C++26 fork

## The release gate (issues #168, #283)

`cxx26/dev/release-gate.sh` is the last verification step before a toolchain candidate is promoted. It runs every
step in a fixed order, gates each on its exit status and prints one summary table with the revision. Nothing is
promoted unless the whole table is green.

    cxx26/dev/release-gate.sh --list                     # the plan
    DRAFT_DIR=<cplusplus/draft checkout at tag n5050> cxx26/dev/release-gate.sh     # everything, at nproc jobs
    STEPS="build clang" cxx26/dev/release-gate.sh        # a subset
    STDS="c++26 c++20" STEPS="libcxx-c++26 libcxx-c++20" cxx26/dev/release-gate.sh

Steps, in order: `build` (all clang tools in build-nyx, `cxx-tidy` and `cxx-test-depends` in build-libcxx), `clang`
(check-clang), `clangd` (check-clangd and ClangdTests), `libcxx-<std>` for c++26, 23, 20, 17, 14, 11, 03 (one lit
run per test directory, `sweep-libcxx-std.sh`, tallied by `sweep-summary.py`, which parses both spellings of the
lit totals), `modules` (the std / std.compat export drift tests at c++23 and c++26) and `audit` (the vocabulary audit
of #283, `libcxx/utils/vocabulary_audit.py`, against the draft at tag n5050; it runs last on purpose). The script refuses to run on a dirty tree or with less than
3 GB free, never run it while another build or lit run uses the build trees, and do not edit the sources while it
runs (the lit runs reinstall the headers from the working tree).

## Probes generated from the draft (issue #116)

`cxx26/dev/synopsis-probes/` has generators that read the LaTeX synopses of the draft and write C++ probes (see its
README). The committed tests `pstl_policy_overloads`, `pstl_ranges_policy_overloads` and
`constexpr_algorithm_overloads` in libcxx/test/std/algorithms/algorithms.general are their output.

## Other tools

`sweep-libcxx-std.sh <std> <out>` (the per-directory libc++ sweep, `JOBS` overrides nproc),
`sweep-summary.py` (totals of a sweep log; non-zero on any failure), `check-module-exports.sh`,
`check-test-tiers.py`, `watch-sweep.sh`, `tidy-headers.py`, `testrun.sh` / `testdiff.py`, `lint-tools/`.
