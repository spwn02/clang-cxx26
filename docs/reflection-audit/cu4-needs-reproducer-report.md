# CU4 — needs-reproducer upstream issues

Date: 2026-09-11

## Result

Items #187 and #212 are fixed with permanent regressions. #184 and #208 are
already fixed/could not reproduce and have regressions. #253 could not be
reproduced because its only attachment is a preprocessed translation unit tied
to an incompatible 21-era `<meta>` ABI. #275 remains open: its pinned source
reproduces two clangd crashes; the first serialization assert has a small
obvious fix, but applying it exposes a second independent infinite recursion
in clangd's body indexer, so no partial fix was retained.

## Individual dispositions

| Issue | Disposition | Evidence |
| --- | --- | --- |
| #184 | Already fixed | Recovered CE source (template NTTP of `info`, expansion statement, runtime use of `identifier_of(F)`) compiles under `-freflection-latest`; regression `issue-184-consteval-only-context.pass.cpp` passes. |
| #187 | Fixed | Exact recovered Godbolt source asserted in `ASTContext::getMemberPointerType`: a dependent splice qualifier is canonical but `MemberPointerType::isSugared()` called it sugar, making its canonical type recurse to itself. Dependent splice qualifiers now return non-sugared. Regression `issue-187-spliced-member-pointer.pass.cpp` passes and executes. |
| #208 | Already fixed | Recovered CE source's reflection-derived array bound (`Structure<DummyStruct>`) compiles as a constant expression; regression `issue-208-crtp-constexpr.pass.cpp` passes. The actual CE program is not CRTP despite the issue title. |
| #212 | Fixed | Corrected recovered CE source reached `TemplateDeclInstantiator::VisitVarDecl` from a `ConstevalBlockDecl`; the NRVO branch assumed every context was a function/block and hit `llvm_unreachable`. A consteval block has no return type and no semantically meaningful NRVO candidate, so the branch now runs only for functions/blocks. Regression `issue-212-optional-extraction.pass.cpp` passes. |
| #253 | Could not reproduce | Downloaded `templateForInModuleICE.zip`; its only source is preprocessed against clang-p2996 21's old `<meta>` ABI. Compiling it against this fork reaches expected API-signature/type errors before the claimed ICE, with no crash. No faithful source/module input remains in the report. |
| #275 | Open, escalated | Fetched GitLab revision `62597e0`. With a reconstructed libc++ reflection compilation database, `clangd --check` first asserts serializing a record-nested `ConstevalBlockDecl` (`Decl::AccessDeclContextCheck`). A local exemption proves that layer, but immediately exposes a distinct stack-overflow recursion in `clangd::BodyIndexer` traversing reflection/function nodes. The exemption was reverted: it would not meet the full-fix bar. |

## Commands and results

* `curl -fsSL .../api/shortlinkinfo/{e4bdnvEEq,ej9EqxvqW,fs1ETG7v4,4r9ebc6zP}`: recovered source states for #187/#184/#208/#212.
* `ninja -C build-nyx -j$(nproc)`: passed after #187/#212 implementation.
* `ninja -C build-libcxx -t clean cxx && ninja -C build-libcxx -j$(nproc) cxx`: passed (initial sandboxed invocation was blocked only by ccache's read-only external cache; approved retry passed).
* `libcxx/utils/libcxx-lit build-libcxx -sv` on the four new regressions: 4/4 passed.
* Exact #187 source with the staged libc++ harness: baseline assertion; fixed compiler exits 0 within 60 seconds.
* #253 attachment under `clang++ -std=c++26 -freflection-latest -x c++-module -fsyntax-only`: expected incompatible old-header errors, no ICE.
* #275 pinned project under `clangd --check` with reconstructed compile database: reproducible assertion, then (with that assertion bypassed) reproducible `BodyIndexer` stack overflow; no retained partial patch.

Broader required suite gates were not completed in this run.
