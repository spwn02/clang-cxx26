#!/usr/bin/env python3
"""Does clangd survive a build rewriting a module file it has open? (issue #487)

usage: repro.py <clangd> <clang++> [--in-place | --truncate] [--workaround | --copy]

Builds module M with <clang++>, opens Use.cpp (import M) in clangd with a compile
database that lists the module file via -fmodule-file=M=..., then rewrites M.pcm
(by default the way a build system's compiler does it now: new file renamed over the
old one; with --in-place: truncated and rewritten, which pulls the file out from under
a memory mapping) and asks clangd for hover/definition of names that come from the
module. Exit status 0: clangd answered and is still running; 1: it died (SIGBUS).
"""
import json, os, shutil, subprocess, sys, tempfile, time

clangd, clangxx = sys.argv[1], sys.argv[2]
in_place = "--in-place" in sys.argv
workaround = "--workaround" in sys.argv  # response file as CMake writes it + .clangd Remove: "@*"
copy = "--copy" in sys.argv  # .clangd: CompileFlags: CopyModuleFiles: true
truncate = "--truncate" in sys.argv  # leave the file half written (a compiler killed mid-write)

work = tempfile.mkdtemp(prefix="clangd-module-rewrite-")
try:
    open(f"{work}/M.cppm", "w").write(
        "export module M;\nexport inline int twice(int x) { return x * 2; }\n"
        "export struct S { int a; int b() const { return a; } };\n"
        + "".join(f"export inline int f{i}(int x) {{ return x + {i}; }}\n" for i in range(4000)))
    use = "import M;\nint g(int n) { S s{n}; return twice(s.b()) + f3999(n) + f2000(n); }\n"
    open(f"{work}/Use.cpp", "w").write(use)
    pcm = f"{work}/M.pcm"
    def build():
        subprocess.run([clangxx, "-std=c++20", "--precompile", f"{work}/M.cppm", "-o", pcm + ".new"], check=True)
        return pcm + ".new"
    shutil.move(build(), pcm)
    if workaround:
        open(f"{work}/M.modmap", "w").write(f"-fmodule-file=M={pcm}\n")
        flags = f"-std=c++20 @{work}/M.modmap -fsyntax-only"
        open(f"{work}/.clangd", "w").write('CompileFlags:\n  Remove: "@*"\n')
        db = [{"directory": work, "command": f"{clangxx} {flags} {work}/Use.cpp", "file": f"{work}/Use.cpp"},
              {"directory": work, "command": f"{clangxx} -std=c++20 @{work}/M.modmap --precompile {work}/M.cppm", "file": f"{work}/M.cppm"}]
    else:
        flags = f"-std=c++20 -fmodule-file=M={pcm} -fsyntax-only {work}/Use.cpp"
        db = [{"directory": work, "command": f"{clangxx} {flags}", "file": f"{work}/Use.cpp"}]
    if copy:
        open(f"{work}/.clangd", "w").write("CompileFlags:\n  CopyModuleFiles: true\n")
    json.dump(db, open(f"{work}/compile_commands.json", "w"))

    p = subprocess.Popen([clangd, "--log=error", f"--compile-commands-dir={work}"],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=(None if os.environ.get("REPRO_STDERR") else subprocess.DEVNULL))
    nid = 0
    def send(obj):
        b = json.dumps(obj).encode()
        p.stdin.write(b"Content-Length: %d\r\n\r\n" % len(b) + b); p.stdin.flush()
    def read():
        hdr = b""
        while not hdr.endswith(b"\r\n\r\n"):
            c = p.stdout.read(1)
            if not c: return None
            hdr += c
        n = int([l for l in hdr.split(b"\r\n") if l.lower().startswith(b"content-length")][0].split(b":")[1])
        return json.loads(p.stdout.read(n))
    def request(method, params):
        global nid; nid += 1; send({"jsonrpc": "2.0", "id": nid, "method": method, "params": params})
        while True:
            m = read()
            if m is None: return None
            if m.get("id") == nid and "method" not in m: return m
    uri = "file://" + f"{work}/Use.cpp"
    request("initialize", {"processId": None, "rootUri": "file://" + work, "capabilities": {}})
    send({"jsonrpc": "2.0", "method": "initialized", "params": {}})
    send({"jsonrpc": "2.0", "method": "textDocument/didOpen", "params": {"textDocument": {"uri": uri, "languageId": "cpp", "version": 1, "text": use}}})
    def hover(line, ch):
        return request("textDocument/hover", {"textDocument": {"uri": uri}, "position": {"line": line, "character": ch}})
    r = hover(1, 32)  # twice
    assert r is not None and r.get("result"), "no hover before the rewrite: %r" % (r,)

    if truncate:
        with open(pcm, "r+b") as f:
            f.truncate(os.path.getsize(pcm) // 2)
    elif in_place:
        data = open(pcm, "rb").read()
        with open(pcm, "r+b") as f:  # what the old compiler did: truncate and rewrite
            f.truncate(0); f.flush(); time.sleep(0.2); f.write(data)
    else:
        shutil.move(build(), pcm)    # what it does now: write a new file, rename over
    time.sleep(0.5)

    ok = True
    try:
      send({"jsonrpc": "2.0", "method": "textDocument/didChange", "params": {"textDocument": {"uri": uri, "version": 2},
            "contentChanges": [{"text": use + "// edit\n"}]}})
      for pos in ((1, 32), (1, 38), (1, 15), (1, 50), (1, 63)):
          r = hover(*pos)
          if r is None or p.poll() is not None:
              ok = False; break
    except (BrokenPipeError, OSError):
        ok = False
    time.sleep(0.5)
    alive = p.poll() is None
    print("answered after the rewrite:", ok, "| clangd alive:", alive, "| exit:", p.poll())
    if alive:
        request("shutdown", None); send({"jsonrpc": "2.0", "method": "exit"})
    sys.exit(0 if ok and alive else 1)
finally:
    shutil.rmtree(work, ignore_errors=True)
