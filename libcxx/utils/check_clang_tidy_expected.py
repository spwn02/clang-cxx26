#!/usr/bin/env python3
# ===----------------------------------------------------------------------===##
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===##

"""Compares the diagnostics of a run-clang-tidy run with a list of expected ones.

Each diagnostic is identified by its file name (without directories), the check
that produced it and its message, and the list gives how many distinct places
produce it ("| xN"); line numbers are not part of the key, so the list does not
change when a file is edited elsewhere. The comparison is strict: a finding that
is not listed (or that occurs more often than listed) fails, and so does a listed
finding that is no longer produced as often (update the list when a cause is fixed).

usage: check_clang_tidy_expected.py <expected-file> -- <command that runs run-clang-tidy>
(a saved output can be checked with: check_clang_tidy_expected.py <expected-file> --output <file>)
"""

import re
import subprocess
import sys

DIAGNOSTIC = re.compile(r"^(?P<file>\S+?):(?P<line>\d+):(?P<col>\d+): (?P<kind>warning|error): (?P<msg>.*) \[(?P<check>[^\],]+)(?:,[^\]]*)?\]$")


def key(file, check, msg):
    return f"{file.rsplit('/', 1)[-1]} | {check} | {msg}"


def main():
    expected_path = sys.argv[1]
    if sys.argv[2] == "--output":
        output_path, status = sys.argv[3], 0
    else:
        assert sys.argv[2] == "--"
        run = subprocess.run(sys.argv[3:], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
        output_path, status = None, run.returncode
        output = run.stdout
    expected = {}
    with open(expected_path) as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#"):
                k, _, count = line.rpartition(" | x")
                expected[k] = int(count)

    places = {}
    if output_path:
        with open(output_path, errors="replace") as f:
            output = f.read()
    for line in output.splitlines():
        m = DIAGNOSTIC.match(line)
        if m:
            places.setdefault(key(m["file"], m["check"], m["msg"]), set()).add((m["file"], m["line"]))
    seen = {k: len(v) for k, v in places.items()}

    failed = False
    for k in sorted(seen):
        if seen[k] > expected.get(k, 0):
            where = ", ".join(f"{f}:{l}" for f, l in sorted(places[k]))
            print(f"unexpected clang-tidy diagnostic ({seen[k]} found, {expected.get(k, 0)} expected): {k} [{where}]")
            failed = True
    for k in sorted(expected):
        if seen.get(k, 0) < expected[k]:
            print(f"fewer clang-tidy diagnostics than expected ({seen.get(k, 0)} found, {expected[k]} expected; update {expected_path}): {k}")
            failed = True
    if status != 0 and not seen:
        print(f"run-clang-tidy failed (exit status {status}) without any diagnostic:\n{output}")
        failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
