# Needs-Build-To-Verify Audit

Date: 2026-09-09

Audited all 27 M1 entries against current HEAD with direct `clang-22 -fsyntax-only` probes where
the issue supplied usable source. The compiler Reflection lit subset passed 20/20; focused
libc++ reflection tests passed 2/2 through `libcxx-lit`.

Results:

- Already-Fixed: 12
- Confirmed-Open: 9
- Not-Applicable/Out-of-Scope: 0
- Still Needs-Build-To-Verify: 6

Confirmed-Open M4 backlog: 10 total, consisting of pre-existing #150 plus audited #169, #180,
#181, #188, #220, #221, #237, #239, and #346.

Remaining Needs-Build-To-Verify entries are #184, #187, #208, #212, #253, and #275. Their reports
did not contain a runnable reproducer: inaccessible Godbolt-only cases, an unavailable module
attachment, or an external clangd project.

Probe sources are tracked under `docs/reflection-audit/repros/nbtv-*.cpp`.
