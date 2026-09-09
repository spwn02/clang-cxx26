Implemented and committed P3293R3 piece 2.

- Supports `obj.[:base:]` for direct non-virtual bases.
- Emits checked derived-to-base casts with cv/value-category preservation.
- Rejects virtual bases and array elements with diagnostics.
- Added positive/negative tests and updated trackers/report.

Verification:

- libc++ focused test: 1/1 passed.
- Full Clang gate: 44,612 passed; 5 established baseline failures.
- Final stale-PCH/tool and reflection rerun: 21/21 passed.

Commit: `1c19c7b1b0f0`