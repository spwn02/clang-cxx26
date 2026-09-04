#!/usr/bin/env python3
"""Diff two archived lit JSON results (see testrun.sh), refusing to compare
across mismatched build configs so the comparison stays meaningful.

Usage:
  testdiff.py <baseline.json> <candidate.json> [--allow-config-mismatch]

Each archived result has a sibling <name>.meta.json (written by testrun.sh)
carrying cxx26_sha / nyx_config_fp / libcxx_config_fp / clang_version. By
default this script refuses to diff two results whose config fingerprints
differ (a different CMakeCache means "new failure" and "different compiler"
are indistinguishable) -- pass --allow-config-mismatch to override, e.g. when
deliberately diffing across a milestone that legitimately changed the config.
"""
import json
import sys
import argparse
from pathlib import Path


def load(path):
    p = Path(path)
    with open(p) as f:
        data = json.load(f)
    # foo.json -> foo.meta.json
    meta_path = p.parent / (p.name[: -len(".json")] + ".meta.json")
    meta = {}
    if meta_path.exists():
        with open(meta_path) as f:
            meta = json.load(f)
    results = {}
    for t in data.get("tests", []):
        results[t["name"]] = t["code"]
    return results, meta


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("baseline")
    ap.add_argument("candidate")
    ap.add_argument("--allow-config-mismatch", action="store_true")
    args = ap.parse_args()

    base, base_meta = load(args.baseline)
    cand, cand_meta = load(args.candidate)

    if base_meta and cand_meta and not args.allow_config_mismatch:
        for key in ("nyx_config_fp", "libcxx_config_fp"):
            bv, cv = base_meta.get(key), cand_meta.get(key)
            if bv and cv and bv != cv:
                print(f"REFUSED: config fingerprint mismatch on {key}: "
                      f"{bv} (baseline) vs {cv} (candidate)", file=sys.stderr)
                print("Pass --allow-config-mismatch to override.", file=sys.stderr)
                sys.exit(2)

    print(f"baseline: {base_meta.get('cxx26_sha', '?')[:12]} "
          f"({base_meta.get('stamp_utc', '?')}) -- {len(base)} tests")
    print(f"candidate: {cand_meta.get('cxx26_sha', '?')[:12]} "
          f"({cand_meta.get('stamp_utc', '?')}) -- {len(cand)} tests")

    failing_codes = {"FAIL", "TIMEOUT"}
    passing_codes = {"PASS", "XFAIL"}

    new_failures = []
    newly_fixed = []
    changed_other = []

    all_names = set(base) | set(cand)
    for name in sorted(all_names):
        b = base.get(name)
        c = cand.get(name)
        if b == c:
            continue
        if b is None:
            # test didn't exist in baseline -- new test, not a regression signal
            continue
        if c is None:
            print(f"MISSING in candidate: {name} (was {b})")
            continue
        if b in passing_codes and c in failing_codes:
            new_failures.append((name, b, c))
        elif b in failing_codes and c in passing_codes:
            newly_fixed.append((name, b, c))
        else:
            changed_other.append((name, b, c))

    print(f"\n=== NEW FAILURES ({len(new_failures)}) ===")
    for name, b, c in new_failures:
        print(f"  {name}: {b} -> {c}")

    print(f"\n=== NEWLY FIXED ({len(newly_fixed)}) ===")
    for name, b, c in newly_fixed:
        print(f"  {name}: {b} -> {c}")

    if changed_other:
        print(f"\n=== OTHER CHANGES ({len(changed_other)}) ===")
        for name, b, c in changed_other:
            print(f"  {name}: {b} -> {c}")

    if new_failures:
        sys.exit(1)


if __name__ == "__main__":
    main()
