#!/usr/bin/env bash
# usage: run-all.sh [clangd]   (default: build-nyx/bin/clangd of this tree)
# Runs every scenarios/<name>/ (t.cpp, flags, steps.json, expect) through replay.sh and
# checks the expectations: lines "<label> must|never <item>" (completion items) or "<label> must-contain|never-contain <text>" (substring of the answer to an LSP request step), compared to the items that
# blink received at the step with that report label. Exit status 1 on any failure.
set -uo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
clangd="${1:-$here/../../../build-nyx/bin/clangd}"
status=0
repo="$(cd "$here/../../.." && pwd)"
for dir in "$here"/scenarios/*/; do
  name="$(basename "$dir")"
  extra_env=(); [[ -f "$dir/env" ]] && mapfile -t extra_env < "$dir/env"
  out="$(env "${extra_env[@]}" "$here/replay.sh" "$clangd" "$dir/t.cpp" "$(cat "$dir/steps.json")" $(sed "s|@REPO@|$repo|g" "$dir/flags"))"
  while read -r label verb text; do
    [[ -z "${label:-}" || "$label" == \#* ]] && continue
    raw="$(grep -F "$label [" <<<"$out" | head -1)"
    # (A step that completed with no items is reported, with an empty list.)
    if [[ -z "$raw" ]]; then echo "FAIL $name: no step reported '$label'"; status=1; continue; fi
    line="$(sed 's/^[^:]*: //' <<<"$raw")"
    norm="|$(sed -E 's/ *\| */|/g; s/^ +//; s/ +$//' <<<"$line")|"
    found=no; [[ "$norm" == *"|$text|"* ]] && found=yes
    if [[ $verb == must-contain || $verb == never-contain ]]; then
      found=no; [[ "$line" == *"$text"* ]] && found=yes
      verb="${verb%-contain}"
    fi
    if [[ $verb == must && $found == no ]] || [[ $verb == never && $found == yes ]]; then
      echo "FAIL $name: $label $verb '$text' (items: $line)"; status=1
    fi
  done < "$dir/expect"
  echo "done $name"
done
exit $status
