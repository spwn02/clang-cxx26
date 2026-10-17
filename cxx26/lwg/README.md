# LWG issue drafts

Submission-ready texts for the library working group issues found while implementing the C++26 working draft in this fork. Wording is relative to N5050 (2026-06-01) and has been checked against the draft's LaTeX source. The trackers on GitHub stay open until the issues are submitted and acted on. Re-verified 2026-10-17 against draft `main` (47974a2f, 2026-10-15): the clauses of #249, #252, #256, #258 and #263 are unchanged (one editorial markup change in [exec.snd.expos]); in #251's [linalg.syn] only the return type of `vector_idx_abs_max` changed (LWG fix, `size_type`, already what libc++ declares), which does not touch the solve overloads.

| tracker | file | issues |
|---|---|---|
| #249 | 249-submdspan-mapping-rank-one-padded.md | 1 (two related defects) |
| #251 | 251-linalg-trsv-trsm-ambiguity.md | 1 |
| #252 | 252-set-difference-parallel-return-position.md | 1 |
| #256 | 256-set-symmetric-difference-undefined-K.md | 1 (editorial) |
| #258 | 258-format-nullptr-constexpr-formatter-unreachable.md | 1 |
| #263 | 263-execution-completion-queries.md | 5 (A-E) |
| #265 | 265-execution-get-awaiter.md | none required (closed; the fork uses `__builtin_get_awaiter`; one optional clarification) |
