#!/usr/bin/env bash
# Progress watchdog for a background lit sweep. One stdout line per event.
f="$1"; pidpat="$2"; total="${3:-64}"
cd /home/spawn/dev/toolchains/clang-cxx26
seen=0; last_change=$(date +%s); last_hb=0; alive_tag=""
while true; do
  now=$(date +%s)
  n=$(grep -c '::' "$f" 2>/dev/null || echo 0)
  if [ "$n" -gt "$seen" ]; then
    # new directory results since last look
    tail -n +"$((seen+1))" "$f" | grep '::' | while IFS= read -r l; do
      d=${l%% ::*}
      if echo "$l" | grep -qE '\|  Failed *:|FAIL:|Unresolved|XPASS|XPass *:'; then
        echo "FAILURE in ${d#libcxx/test/}: $(echo "$l" | cut -c1-260)"
      fi
    done
    seen=$n; last_change=$now
  fi
  # newest activity anywhere in the lit exec tree (seconds ago)
  act=$(find build-libcxx/libcxx/test -newermt '-3 minutes' -type f 2>/dev/null | head -1)
  [ -n "$act" ] && last_change=$now
  running=$(pgrep -f "$pidpat" >/dev/null && echo yes || echo no)
  if grep -q SWEEP-DONE "$f" 2>/dev/null; then echo "DONE: $n directories; failures: $(grep -cE '\|  Failed *:|FAIL:|Unresolved|XPASS|XPass *:' "$f")"; exit 0; fi
  if [ "$running" = no ]; then echo "DEAD: sweep process is gone after $n directories without SWEEP-DONE"; exit 1; fi
  idle=$((now-last_change))
  if [ "$idle" -gt 900 ]; then echo "STALL: no new directory and no build-tree activity for $((idle/60)) min (dir $n/$total, last: $(grep '::' "$f" | tail -1 | cut -c1-60))"; last_change=$now; fi
  if [ $((now-last_hb)) -ge 600 ]; then echo "HEARTBEAT: $n/$total directories done, last: $(grep '::' "$f" | tail -1 | cut -d: -f1 | sed 's#libcxx/test/##'), $(df -h /home | tail -1 | awk '{print $4}') free"; last_hb=$now; fi
  sleep 20
done
