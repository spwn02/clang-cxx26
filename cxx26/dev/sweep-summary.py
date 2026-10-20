#!/usr/bin/env python3
# Tally a log written by sweep-libcxx-std.sh (one line per test directory:
# "<dir> :: <lit summary fields joined by '|'>") and fail on anything but passed, unsupported and
# expected failures.
#
# lit prints the counts as "  Failed: 1 (100.00%)" when a run has a single kind of result and as
# "  Failed : 1 (0.01%)" (padded) when it has several, so a pattern for one spelling misses the other
# (a watchdog grep once let two failures through that way). Every spelling is parsed here.
#
# usage: sweep-summary.py <sweep log>...    exit status 1 if a failure, unresolved test or unexpected pass
#        is present, or if a log has no SWEEP-DONE line (the sweep did not finish).
import re
import sys

KINDS = ("Passed", "Unsupported", "Expectedly Failed", "Failed", "Unresolved", "Unexpectedly Passed", "XPass")
BAD = ("Failed", "Unresolved", "Unexpectedly Passed", "XPass")


def tally(path):
    counts = dict.fromkeys(KINDS, 0)
    total = 0
    bad_dirs = []
    done = False
    for line in open(path, errors="replace"):
        if line.startswith("SWEEP-DONE"):
            done = True
            continue
        if " :: " not in line:
            continue
        d = line.split(" :: ", 1)[0]
        m = re.search(r"Total Discovered Tests: (\d+)", line)
        if m:
            total += int(m.group(1))
        for kind in KINDS:
            # "|  Failed : 3 (..)", "|  Failed: 3 (..)", also tolerant of padding before the colon.
            m = re.search(r"\|\s*" + re.escape(kind) + r"\s*:\s*(\d+)", line)
            if m:
                counts[kind] += int(m.group(1))
                if kind in BAD and int(m.group(1)):
                    bad_dirs.append((d, kind, int(m.group(1))))
        if re.search(r"(^|\|)\s*(FAIL|UNRESOLVED|XPASS):", line):
            bad_dirs.append((d, "FAIL-line", 1))
    return total, counts, bad_dirs, done


def main(argv):
    status = 0
    for path in argv[1:]:
        total, counts, bad, done = tally(path)
        accounted = sum(counts[k] for k in ("Passed", "Unsupported", "Expectedly Failed", "Failed", "Unresolved"))
        line = f"{path}: {total} tests; " + ", ".join(f"{k} {v}" for k, v in counts.items() if v)
        if not done:
            line += "; INCOMPLETE (no SWEEP-DONE)"
            status = 1
        if accounted != total:
            # A directory with no tests of its own (data directories) prints nothing; any other gap is a lost result.
            line += f"; note: {total - accounted} tests not accounted for"
        print(line)
        for d, kind, n in bad:
            print(f"  BAD {kind} x{n}: {d}")
            status = 1
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv))
