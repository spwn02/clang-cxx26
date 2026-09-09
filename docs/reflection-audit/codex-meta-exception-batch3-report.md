Implemented and pushed batch 3.

- Added wrappers: `template_of`, `template_arguments_of`, `access_context::via`, `enumerators_of`.
- Preserved prior six wrappers: 10 total.
- Reverted unsafe member/parameter guards; reserved for strategy 2.
- Final libc++ reflection gate: 75 passed, 1 unsupported, 0 failed.
- Added [batch-3 report](</home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-meta-exception-batch3-report.md>).
- Final commit: `f90e23607f9a`, pushed to `origin/cxx26`.

Pre-existing modification remains in `codex-meta-exception-batch2-report.md`; it was not altered or staged.