import re,collections,sys,pickle
pos=re.compile(r'^# \| (\S+?):(\d+):(\d+): (?:error|warning): (.*)\[([\w.,-]+?)(?:,-warnings-as-errors)?\]$')
seen={}
for ln in open(sys.argv[1],errors='replace'):
    m=pos.match(ln.rstrip('\n'))
    if not m: continue
    f=re.sub(r'^.*c\+\+/v1/','',m.group(1)); f=re.sub(r'^.*libcxx/include/','',f)
    seen[(f,int(m.group(2)),int(m.group(3)),m.group(5).split(',')[0])]=m.group(4)
chk=collections.Counter(k[3] for k in seen)
print(len(seen),chk.most_common())
pickle.dump(seen,open(sys.argv[2],'wb'))
