cd /home/spawn/dev/toolchains/clang-p2996
I=build-libcxx/libcxx/test-suite-install/include/c++/v1
for h in "$@"; do
for i in 1 2 3 4 5; do
  cp -r libcxx/include/. $I/
  echo "#include <$h>" > $CLAUDE_JOB_DIR/tmp/one.cpp
  build-nyx/bin/clang++ -std=c++26 -nostdinc++ -I $I -fsyntax-only $CLAUDE_JOB_DIR/tmp/one.cpp 2>&1 | grep "note: previous declaration is here" | sed -E 's/^([^:]+):([0-9]+):.*/\1 \2/' | sort -u > $CLAUDE_JOB_DIR/tmp/prev.txt
  n=$(wc -l < $CLAUDE_JOB_DIR/tmp/prev.txt); echo "$h iter$i prev=$n"; [ $n = 0 ] && break
  python3 - <<'P'
import os,re,collections
d=collections.defaultdict(list)
for ln in open(os.environ['CLAUDE_JOB_DIR']+'/tmp/prev.txt'):
    f,l=ln.split(); d[f].append(int(l))
for f,ls in d.items():
    f=re.sub(r'^.*c\+\+/v1/','libcxx/include/',f)
    L=open(f).read().split('\n')
    for l in ls:
        s=L[l-1]
        if '_LIBCPP_HIDE_FROM_ABI' in s: continue
        m=re.match(r'^(\s*)(.*)$',s); L[l-1]=m.group(1)+'_LIBCPP_HIDE_FROM_ABI '+m.group(2)
    open(f,'w').write('\n'.join(L))
P
done; done
