#!/usr/bin/env python3
"""Compile every translation unit of clang/ and clang-tools-extra/ with C++20 extensions as errors.

LLVM is built as C++17, and MSVC (which has no designated initializers or other C++20 extensions in /std:c++17) builds
it too. Clang accepts them as an extension, so the Linux build does not notice; this scan does.

    cxx26/dev/scan-cxx20-extensions.py build-nyx          # build directory with a compile_commands.json

Prints one line per translation unit with a hit and exits 1 if there is any.
"""
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys

build = sys.argv[1] if len(sys.argv) > 1 else "build-nyx"
commands = json.load(open(os.path.join(build, "compile_commands.json")))
entries = [e for e in commands
           if re.search(r"/(clang|clang-tools-extra)/", e["file"]) and "/test/" not in e["file"]
           and "/unittests/" not in e["file"] and e["file"].endswith(".cpp")]
print("translation units:", len(entries), flush=True)


def run(entry):
    cmd = re.sub(r" -o \S+", " -o /dev/null", entry["command"])
    cmd = re.sub(r" -c ", " -fsyntax-only ", cmd, count=1)
    r = subprocess.run(cmd + " -Wno-everything -Werror=c++20-designator -Werror=c++20-extensions -ferror-limit=0 "
                       "-fno-color-diagnostics", shell=True, cwd=entry["directory"], capture_output=True, text=True)
    return entry["file"], sorted({l for l in r.stderr.split("\n") if "error:" in l and "c++20" in l.lower()})


hits = 0
with cf.ThreadPoolExecutor(os.cpu_count()) as pool:
    for path, errors in pool.map(run, entries):
        for line in errors:
            hits += 1
            print(line, flush=True)
print("hits:", hits)
sys.exit(1 if hits else 0)
