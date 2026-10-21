#!/usr/bin/env bash
# Vocabulary audit (#283) at the older language modes. Usage: audit-older-modes.sh <draft dir> <out dir>
#
# libc++ exposes later-standard names in older modes by design (retroactive defect reports, C99 names,
# forward declarations), so the audit at c++23..c++11 is compared against upstream instead of being empty:
# for each mode the audit runs against that standard's draft tag, only `name` and `macro` entries are
# kept (the header tables of old drafts use a different markup), and an entry fails unless its last
# component also appears in the headers of upstream libc++ (BASELINE, default llvmorg-22.1.8). What
# remains is a name the fork added on its own. The draft checkout is restored to DRAFT_TAG on exit.
set -uo pipefail
draft="$1"; out="$2"
baseline="${BASELINE:-llvmorg-22.1.8}"
restore="${DRAFT_TAG:-n5050}"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p "$out"
trap 'git -C "$draft" checkout -q "$restore"' EXIT
git rev-parse -q --verify "$baseline" >/dev/null || { echo "baseline tag $baseline is missing" >&2; exit 2; }

lcx=build-libcxx
inc="$lcx/libcxx/test-suite-install/include/c++/v1"; extra=()
for d in "$lcx"/libcxx/test-suite-install/include/*/c++/v1; do [[ -d "$d" ]] && extra+=(--extra-include "$d"); done

fail=0
for p in "c++23 n4950" "c++20 n4861" "c++17 n4659" "c++14 n4140" "c++11 n3337"; do
  set -- $p; std="$1"; tag="$2"
  git -C "$draft" checkout -q "$tag" || { echo "$std: cannot check out $tag" >&2; fail=1; continue; }
  python3 -I libcxx/utils/vocabulary_audit.py --draft "$draft" --sd6 libcxx/utils/sd6_lib_macros.txt \
    --allow libcxx/utils/vocabulary_allowlist.txt --clang build-nyx/bin/clang++ --include "$inc" \
    ${extra[@]+"${extra[@]}"} --std "$std" >"$out/audit-$std.txt" 2>&1
  added=0
  while read -r kind entry _; do
    [[ "$kind" == name || "$kind" == macro ]] || continue
    last="${entry##*::}"
    if ! git grep -qw -- "$last" "$baseline" -- libcxx/include; then
      echo "$std: $kind $entry is not in upstream $baseline (fork-added)"; added=$((added + 1))
    fi
  done <"$out/audit-$std.txt"
  total=$(grep -cE '^(name|macro) ' "$out/audit-$std.txt")
  echo "$std ($tag): $total name/macro entries, all in upstream except $added"
  (( added == 0 )) || fail=1
done
exit "$fail"
