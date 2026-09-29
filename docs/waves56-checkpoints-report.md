# `std::observable_checkpoint` implementation report

## Implemented

- Added `llvm.observable.checkpoint`, with opaque inaccessible-memory effects,
  `NoDuplicate`/`NoMerge`, and a new intrinsic property that excludes the
  default `WillReturn` attribute. Attributor and legacy function-attribute
  inference also refuse to infer `willreturn` for a function containing a
  checkpoint. LLVM's
  `isGuaranteedToTransferExecutionToSuccessor` treats it as a potential
  termination point. The shared transfer analysis is used by transformations
  that simplify unconditional UB, including InstCombine's backward cleanup of
  instructions before `unreachable`.
- Both SelectionDAG and GlobalISel drop the intrinsic during final instruction
  selection. This leaves it available through the optimizer pipeline, including
  LTO, without machine instructions. FastISel at `-O0` also drops it.
- Added `__builtin_observable_checkpoint()` and libc++'s C++26 freestanding
  `std::observable_checkpoint() noexcept`, exposed from `<utility>`, with
  `__cpp_lib_observable_checkpoint == 202506L`. Both the public declaration
  and macro are gated on `__has_builtin`; libc++ uses a private no-op helper
  when compiling with an older compiler.
- Returning `observe` contract violation handlers have a checkpoint after the
  handler call and before branching back to the contract sequence, including
  the exception-reporting path.
- In C++26 mode, recognized C stdio and wide stdio calls receive checkpoints
  on normal return, covering formatted input/output, character/string I/O,
  read/write, open/close, flush, file positioning, and file
  naming/management. String-buffer formatting functions are excluded.
  Target-dependent `fpos_t`/`wint_t` declarations are recognized by their
  C-linkage names instead of introducing ABI-incompatible builtin prototypes;
  direct `__builtin_` aliases are covered where Clang can declare them safely.
- libc++ print/println add checkpoints after successful output completion.
  `basic_streambuf` adds them after input/output/sync/positioning operations,
  covering iostream paths while remaining inside the streambuf abstraction.
  The internal helper emits only in C++26 mode when the compiler builtin exists.
  `basic_filebuf` marks file open/close operations after they complete.
- Added LLVM optimizer, Attributor, Clang CodeGen, profile/debug, stdio and
  libc++ API/I/O tests, including a ThinLTO O3 optimizer pipeline.

## Validation and remaining work

Integrated focused validation passes for the LLVM InstCombine and Attributor
checks, Clang CodeGen and Contracts checks, and libc++ checkpoint API/I/O
coverage. Checks cover prefix retention before UB, `willreturn` inference,
stdio/wide-stdio boundary insertion, contract handler ordering, debug/profile
locations, optimizer pipelines including LTO, and final machine-code removal.
The broader focused checkpoint subset passed along with the 133-test libc++
feature suite.

The full Clang and libc++ archives show no new feature regressions against the
recorded baselines. The only new suite failures are sandbox restrictions: lit
forkserver sockets, GDB ptrace, and filesystem-test Unix socket binds. Platform
specific I/O entry points remain outside direct Clang builtin recognition;
libc++ print and streambuf paths insert checkpoints at their completed I/O
boundaries. #30 remains open pending final review and publication.
