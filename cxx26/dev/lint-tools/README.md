# lint / probe helpers used for #299, #274 (2026-10-14)

These were written for one-off jobs; paths inside point at /tmp scratch dirs and $CLAUDE_JOB_DIR, adjust before reuse.

- `header-sweep.sh`: compile `#include <H>` for every public header at C++26 and C++20 against build-libcxx/libcxx/test-suite-install (copy libcxx/include there first with `cp -r libcxx/include/. <install>/`). Catches abi_tag redeclaration and missing-declaration errors that lit's header tests do not.
- `run-lit-tidy-on-include-dir.sh <include-dir> <outdir>`: the exact lit RUN line of libcxx/test/libcxx/clang_tidy.gen.py on every generated test, against any include dir (use a pristine upstream copy + __config_site + __assertion_handler for a baseline). Pristine upstream is clean with it; `tidy-headers.py` flags differ and its "548 baseline" was wrong.
- `parse-lit-tidy.py <lit log> <out.pkl>`: parse clang-tidy diagnostics from a lit log into (file,line,col,check).
- `adl-qualify.py`: qualify unqualified helper calls flagged by libcpp-robust-against-adl with the declaring namespace (namespace tracker); verify with header-sweep.sh AND tests: a qualified call to a later-declared function only breaks when instantiated (hive erase_if).
- `older-modes-gate.sh`, `modules-include-loop.sh`: older-mode lit gates; the clang-modules include tests write a huge std PCM each (ENOSPC on a 97% full disk: run with --filter one at a time and delete Output between).
- `cx.sh`, `cxrun.sh`: compile / run a probe with the build tree's libc++ (use these instead of flag strings in zsh).
