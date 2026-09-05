#!/usr/bin/env python3
"""Anti-regression gate for the test-coverage shape that let the Contracts
Hardening epic's headline bug ship (docs/CONTRACTS_HARDENING.md M4): a test
file that *looks* like it executes a binary and checks a real value, but
either never actually runs anything, or never got past a stub. Flags three
patterns, all confirmed true of clang/test/Contracts/Runnable/
contract-result-name.cpp before this epic fixed it:

  1. A file with `int main(` whose RUN lines never invoke the built binary
     (no `%t` run step) -- it looks executable and isn't.
  2. A file under a `Runnable/` directory with no `RUN: %t` execution line
     at all -- these directories exist specifically for tests that execute;
     one that doesn't defeats the point of being there.
  3. A `main()` whose body is empty, or contains only a TODO/FIXME comment
     (nothing else) -- a stub that was never filled in.

This does NOT flag a -verify diagnostic test's own main() (rules 1 and 3
below), even one with an empty/incidental body -- a Sema `-verify` test using
main() purely as scaffolding to attach diagnostics to is a common, legitimate
pattern (T0/T1 in this epic's tier model), not the bug shape. A file inside a
Runnable/ directory (rule 2) is held to the execution requirement regardless
of -verify, since being there is a much stronger, unconditional signal of
intent to execute.

Usage: cxx26/dev/check-test-tiers.py <dir> [<dir> ...]
Exits nonzero (and lists every finding) if anything is flagged.
"""
import re
import sys
from pathlib import Path

MAIN_DECL_RE = re.compile(r"\bint\s+main\s*\(")
RUN_LINE_RE = re.compile(r"^\s*//\s*RUN:\s*(.*)$", re.MULTILINE)
RUNS_BINARY_RE = re.compile(r"%t\b")
MAIN_BODY_RE = re.compile(r"\bint\s+main\s*\([^)]*\)\s*\{(.*?)\}", re.DOTALL)
NO_EXEC_RE = re.compile(r"//\s*NO-EXEC:\s*\S")


def strip_comments_and_whitespace(body: str) -> str:
    body = re.sub(r"//[^\n]*", "", body)
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.DOTALL)
    return body.strip()


def check_file(path: Path) -> list[str]:
    findings = []
    text = path.read_text(errors="replace")
    if NO_EXEC_RE.search(text):
        return findings
    run_lines = RUN_LINE_RE.findall(text)
    has_main = MAIN_DECL_RE.search(text) is not None
    runs_binary = any(RUNS_BINARY_RE.search(line) for line in run_lines)
    is_verify_test = any("-verify" in line for line in run_lines)

    if has_main and run_lines and not runs_binary and not is_verify_test:
        findings.append(
            "declares int main() but no RUN line executes %t -- "
            "looks executable, isn't"
        )

    if "Runnable" in path.parts and run_lines and not runs_binary:
        findings.append(
            "under a Runnable/ directory but no RUN line executes %t"
        )

    main_match = MAIN_BODY_RE.search(text)
    if main_match and not is_verify_test:
        stripped = strip_comments_and_whitespace(main_match.group(1))
        if stripped == "" or re.fullmatch(r"(TODO|FIXME)[^;{}]*", stripped, re.IGNORECASE):
            findings.append("main() body is empty or only a TODO/FIXME stub")

    return findings


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 1

    all_findings: dict[Path, list[str]] = {}
    for root_arg in argv[1:]:
        root = Path(root_arg)
        paths = [root] if root.is_file() else sorted(root.rglob("*.cpp"))
        for path in paths:
            findings = check_file(path)
            if findings:
                all_findings[path] = findings

    if not all_findings:
        print(f"check-test-tiers: clean ({len(argv) - 1} root(s) scanned)")
        return 0

    print("check-test-tiers: found tests shaped like the epic's original gap:")
    for path, findings in sorted(all_findings.items()):
        for finding in findings:
            print(f"  {path}: {finding}")
    print(
        f"\n{len(all_findings)} file(s) flagged. If a flagged file is "
        "deliberately not meant to execute, add a `// NO-EXEC: <reason>` "
        "line to it."
    )
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
