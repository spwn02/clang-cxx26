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
  - Commit: `86b19f56a362`.
  - Verification: `consteval-reentrant-instantiation.pass.cpp` passed; `clang` built at `-j2`;
    final capped direct-lit Clang gate returned exactly the five documented pre-existing failures.
- Issue #303 / upstream PR #306: reopened namespace walks truncated by out-of-line class members.
  - Commit: `7626b6ad000e`.
  - Verification: namespace-member libc++ regression passed; `clang` built at `-j2`; final capped
    direct-lit Clang gate returned exactly the five documented pre-existing failures.
- Issue #311 / upstream PR #315: builtin-template diagnostic ICE.
  - Commit: `aafe2c1ea7b6`.
  - Verification: builtin-template diagnostic libc++ regression passed; `clang` built at `-j2`;
    final capped direct-lit Clang gate returned exactly the five documented pre-existing failures.
- Issue #314 / upstream PR #318: LP64 NEON vector mangling ICE.
  - Commit: `f3480e421c61`.
  - Verification: focused AArch64 NEON mangling test passed; `clang` built at `-j2`; final capped
    direct-lit Clang gate returned exactly the five documented pre-existing failures.
- Issue #321 / upstream PR #323: 32K+ template packs miscompile.
  - Commit: `915d6a93957d`.
  - Verification: 32K+/50K-element `define_static_string` libc++ regression passed; `clang` built
    at `-j2`; final capped direct-lit Clang gate returned exactly the five documented pre-existing
    failures.
- Issue #327 / upstream PR #328: expansion statement ICE on unresolved overload range.
  - Commit: `c1d0c5075bbb`.
  - Verification: focused overload-range diagnostic regression passed; `clang` built at `-j2`; final
    capped direct-lit Clang gate returned exactly the five documented pre-existing failures.
- Issue #350 / upstream PR #352: `->[:member:]` assertion with lvalue pointer.
  - Commit: `84ba1ef5ccaa`.
  - Verification: extended `splice-exprs.cpp` regression passed; `clang` built at `-j2`; final capped
    direct-lit Clang gate returned exactly the five documented pre-existing failures.
- Issue #342 / upstream PR #353: `^^derived::operator()` rejects using-declaration.
  - Commit: pending commit for this report update.
  - Verification: wording/using-declarator reflection regression passed; `clang` built at `-j2`; final
    capped direct-lit Clang gate returned exactly the five documented pre-existing failures.

All 12 requested PRs were attempted and closed; no batch items remain.
