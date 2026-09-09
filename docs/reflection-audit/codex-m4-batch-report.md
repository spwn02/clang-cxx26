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
  - Commit: `419d7a1ef581`.
  - Verification: libc++ wrapper regression passed; `clang` built at `-j2`; final capped direct-lit
    Clang gate returned exactly the five documented pre-existing failures.
- Issue #294 / upstream PR #295: invalid formed types during can_substitute/substitute.
  - Commit: `63fd37d6f333`.
  - Verification: both focused libc++ regressions passed; `clang` built at `-j2`; final capped
    direct-lit Clang gate returned exactly the five documented pre-existing failures.
- Issue #288 / upstream PR #289: reentrant constant-evaluation use-after-free.
  - Commit: pending commit for this report update.
  - Verification: `consteval-reentrant-instantiation.pass.cpp` passed; `clang` built at `-j2`;
    final capped direct-lit Clang gate returned exactly the five documented pre-existing failures.

## Not attempted

PRs #306, #315, #318, #323, #328, #352, and #353 remain in the requested
order.
