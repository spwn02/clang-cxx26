#!/usr/bin/env python3
"""RssAnon of clangd with N open files that import one large module (#487).

usage: rss.py <clangd> <clang++> [N=6]

The reader fix copies module files into memory instead of mapping them; this shows
what it costs: private memory (RssAnon) versus shared page cache (RssFile).
"""
import json, os, shutil, subprocess, sys, tempfile, time

clangd, clangxx = sys.argv[1], sys.argv[2]
N = int(sys.argv[3]) if len(sys.argv) > 3 else 6
work = tempfile.mkdtemp(prefix="clangd-rss-")
try:
    open(f"{work}/M.cppm", "w").write("export module M;\n" + "".join(
        f"export inline int f{i}(int x) {{ return x + {i}; }}\nexport struct S{i} {{ int a{i}; }};\n" for i in range(6000)))
    pcm = f"{work}/M.pcm"
    subprocess.run([clangxx, "-std=c++20", "--precompile", f"{work}/M.cppm", "-o", pcm], check=True)
    print("module file: %.1f MB" % (os.path.getsize(pcm) / 1e6))
    db = []
    for i in range(N):
        open(f"{work}/U{i}.cpp", "w").write("import M;\nint g%d(int n) { S%d s{n}; return f%d(s.a%d); }\n" % (i, i, i, i))
        db.append({"directory": work, "command": f"{clangxx} -std=c++20 -fmodule-file=M={pcm} -fsyntax-only {work}/U{i}.cpp", "file": f"{work}/U{i}.cpp"})
    json.dump(db, open(f"{work}/compile_commands.json", "w"))
    p = subprocess.Popen([clangd, "--log=error", f"--compile-commands-dir={work}", "-j=4"],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    def send(o):
        b = json.dumps(o).encode(); p.stdin.write(b"Content-Length: %d\r\n\r\n" % len(b) + b); p.stdin.flush()
    def read():
        h = b""
        while not h.endswith(b"\r\n\r\n"):
            c = p.stdout.read(1)
            if not c: return None
            h += c
        n = int([l for l in h.split(b"\r\n") if l.lower().startswith(b"content-length")][0].split(b":")[1])
        return json.loads(p.stdout.read(n))
    nid = [0]
    def req(m, params):
        nid[0] += 1; send({"jsonrpc": "2.0", "id": nid[0], "method": m, "params": params})
        while True:
            r = read()
            if r is None or (r.get("id") == nid[0] and "method" not in r): return r
    req("initialize", {"processId": None, "rootUri": "file://" + work, "capabilities": {}})
    send({"jsonrpc": "2.0", "method": "initialized", "params": {}})
    def rss():
        d = {}
        for l in open(f"/proc/{p.pid}/status"):
            if l.startswith(("RssAnon", "RssFile")): k, v = l.split(":"); d[k] = int(v.split()[0]) / 1024
        return d
    for i in range(N):
        t = open(f"{work}/U{i}.cpp").read()
        send({"jsonrpc": "2.0", "method": "textDocument/didOpen", "params": {"textDocument": {"uri": f"file://{work}/U{i}.cpp", "languageId": "cpp", "version": 1, "text": t}}})
        r = req("textDocument/hover", {"textDocument": {"uri": f"file://{work}/U{i}.cpp"}, "position": {"line": 1, "character": 20}})
    time.sleep(2)
    r = rss()
    print("N=%d  RssAnon=%.0f MB  RssFile=%.0f MB" % (N, r["RssAnon"], r["RssFile"]))
    req("shutdown", None); send({"jsonrpc": "2.0", "method": "exit"})
finally:
    shutil.rmtree(work, ignore_errors=True)
