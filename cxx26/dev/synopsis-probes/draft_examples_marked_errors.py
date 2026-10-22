#!/usr/bin/env python3
# For every example of the core chapters of the draft whose code has lines marked '// error' / '// ill-formed':
# compile it with the tree's clang and report the marked lines that got no diagnostic (an accepted ill-formed
# example is a candidate bug). usage: examples.py <draft> <outdir>
import re, sys, os, subprocess, concurrent.futures
D, OUT = sys.argv[1:3]
os.makedirs(OUT, exist_ok=True)
CLANG='/home/spawn/dev/toolchains/clang-p2996/build-nyx/bin/clang++'
FILES=['lex','basic','expressions','statements','declarations','classes','overloading','templates','exceptions','modules','cpp','intro','support']
def clean(code):
    code=re.sub(r'@\\caret@','^',code)
    code=code.replace('\\caret','^').replace('\\tcode{','').replace('\\keyword{','')
    code=re.sub(r'@\\[a-zA-Z]+\{([^@]*?)\}@',r'\1',code)
    code=re.sub(r'@\\[a-zA-Z]+@','',code)
    code=re.sub(r'@([^@]*)@',r'\1',code)
    code=code.replace('\\%','%').replace('\\#','#').replace('\\{','{').replace('\\}','}').replace('\\^','^')
    code=code.replace('\\textit{','').replace('$','')
    return code
cases=[]
for f in FILES:
    p=f'{D}/source/{f}.tex'
    if not os.path.exists(p): continue
    t=open(p).read()
    for m in re.finditer(r'\\begin\{example\}(.*?)\\end\{example\}',t,re.S):
        body=m.group(1)
        for cb in re.finditer(r'\\begin\{codeblock\}(.*?)\\end\{codeblock\}',body,re.S):
            code=clean(cb.group(1))
            lines=code.split('\n')
            marked=[i+1 for i,l in enumerate(lines) if re.search(r'//\s*(error|ill-formed|ill formed|not ok|invalid)',l,re.I) and not re.search(r'no error|not an error|OK',l)]
            if marked:
                cases.append((f,len(cases),code,marked))
print(len(cases),'examples with marked lines',file=sys.stderr)
def run(c):
    f,n,code,marked=c
    path=f'{OUT}/{f}_{n}.cpp'
    PRE='#include <meta>\n#include <contracts>\n#include <vector>\n#include <string>\n#include <initializer_list>\n#include <cstddef>\n#include <new>\n#include <typeinfo>\n#include <compare>\n#include <utility>\n#include <type_traits>\n#include <concepts>\n#include <array>\n#include <tuple>\n#include <functional>\n#include <ranges>\n#include <optional>\n#include <memory>\n#include <exception>\n'
    NPRE=PRE.count('\n')
    open(path,'w').write(PRE+code+'\n')
    r=subprocess.run([CLANG,'-std=c++26','-fsyntax-only','-nostdinc++','-isystem','/home/spawn/dev/toolchains/clang-p2996/build-libcxx/libcxx/test-suite-install/include/c++/v1','-ferror-limit=0','-fno-diagnostics-color',path],capture_output=True,text=True)
    diag=set(int(m.group(1))-NPRE for m in re.finditer(re.escape(path)+r':(\d+):\d+: (?:error|fatal error|warning)',r.stderr))
    missing=[l for l in marked if l not in diag]
    return f,n,path,marked,missing,len(diag)
res=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('J','6'))) as ex:
    for r in ex.map(run,cases): res.append(r)
with open(f'{OUT}/report.txt','w') as o:
    for f,n,path,marked,missing,nd in res:
        if missing and nd>=0:
            o.write(f'{f} #{n} {path} marked={marked} missing={missing} ndiag={nd}\n')
print(sum(1 for r in res if r[4]),'examples with a marked line without a diagnostic')
