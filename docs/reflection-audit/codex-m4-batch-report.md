# Reflection Closure Epic — Codex M4 Batch Report

Date: 2026-09-09

## Closed

- Issue #290 / upstream PR #291: entity-proxy member queries and NTTP mangling.
  - Commit: `16a3f705ef1a`.
  - Verification: libc++ wrapper test passed; final capped full Clang gate returned exactly the
    five documented pre-existing failures.
- Issue #276 / upstream PR #277: dependent splice-type canonicalization asymmetry.
  - Commit: `959ec92fdf49`.
  - Verification: `dependent-splice-overloads.cpp` passed; `clang` built at `-j2`; final capped
    direct-lit Clang gate returned exactly the five documented pre-existing failures.
- Issue #309 / upstream PR #310: dependent splice classification during auto NTTP deduction.
  - Commit: pending commit for this report update.
  - Verification: libc++ wrapper regression passed; `clang` built at `-j2`; final capped direct-lit
    Clang gate returned exactly the five documented pre-existing failures.

## Not attempted

PRs #295, #289, #306, #315, #318, #323, #328, #352, and #353 remain in the requested
order.
