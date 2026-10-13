#!/usr/bin/env python3
"""Run libc++'s clang-tidy checks (libcxx/.clang-tidy + the libcxx-tidy plugin) on every public header of
libcxx/include and print the deduplicated diagnostics, one per line: header:line:col: check: message.

Faster to iterate with than libcxx/test/libcxx/clang_tidy.gen.py and independent of the installed headers.
usage: tidy-headers.py [--jobs N] [--header NAME ...] [--out FILE]
"""
import argparse, concurrent.futures, os, pathlib, re, subprocess, sys, tempfile

root = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "libcxx/utils"))
from libcxx.header_information import lit_header_restrictions, lit_header_undeprecations, public_headers

tidy = root / "build-nyx/bin/clang-tidy"
plugin = root / "build-libcxx/libcxx/test/tools/clang_tidy_checks/libcxx-tidy.plugin"
INCLUDE = pathlib.Path(os.environ.get("TIDY_HEADERS_INCLUDE", str(root / "libcxx/include")))
FLAGS = ["-Wweak-vtables", "-pthread", "--target=x86_64-unknown-linux-gnu", "-nostdinc++",
         "-I", str(INCLUDE), "-I", str(root / "libcxx/test/support"), "-std=c++26", "-Wall", "-Wextra", "-Wshadow", "-Wundef",
         "-Wno-unused-command-line-argument", "-Wno-nullability-completeness", "-Wno-pessimizing-move", "-Wno-noexcept-type", "-Wno-atomic-alignment", "-Wno-reserved-module-identifier", "-Wno-tautological-compare", "-Wno-local-type-template-args", "-Wno-c++11-extensions", "-Wno-unknown-pragmas", "-Wno-pass-failed", "-Wno-mismatched-new-delete", "-Wno-redundant-move", "-Wno-self-move", "-Wno-attributes", "-Wno-user-defined-literals",
         "-D_LIBCPP_ENABLE_EXPERIMENTAL", "-D_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER", "-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_NONE",
         "-Wuser-defined-warnings", "-fno-modules", "-flax-vector-conversions=none"]
DIAG = re.compile(r"^(?P<path>/[^:\n]+):(?P<line>\d+):(?P<col>\d+): (?:error|warning): (?P<msg>.*) \[(?P<check>[A-Za-z0-9,.\-]+)\]$")

def run(header, tmp):
    cfgdir = pathlib.Path(tmp) / "config"
    if not cfgdir.exists():
        cfgdir.mkdir(exist_ok=True)
        (cfgdir / "__config_site").write_text((root / "build-libcxx/include/c++/v1/__config_site").read_text())
    src = pathlib.Path(tmp) / (header.replace("/", "_") + ".sh.cpp")
    src.write_text(f"{lit_header_undeprecations.get(header, '')}\n#include <{header}>\n")
    p = subprocess.run([str(tidy), str(src), "--warnings-as-errors=*", "-header-filter=.*",
                        f"--config-file={root}/libcxx/.clang-tidy", f"--load={plugin}", "--"] + FLAGS + ["-I", str(cfgdir)],
                       capture_output=True, text=True)
    out = []
    for l in p.stdout.splitlines() + p.stderr.splitlines():
        m = DIAG.match(l)
        if not m:
            continue
        path = m["path"]
        for base in (str(INCLUDE) + "/",):
            if path.startswith(base):
                path = path[len(base):]
                break
        else:
            continue
        out.append(f"{path}:{m['line']}:{m['col']}: {m['check'].split(',')[0]}: {m['msg']}")
    return out

def fix(headers, checks):
    with tempfile.TemporaryDirectory() as tmp:
        cfgdir = pathlib.Path(tmp) / "config"
        cfgdir.mkdir()
        (cfgdir / "__config_site").write_text((root / "build-libcxx/include/c++/v1/__config_site").read_text())
        srcs = []
        for header in headers:
            src = pathlib.Path(tmp) / (header.replace("/", "_") + ".sh.cpp")
            src.write_text(f"{lit_header_undeprecations.get(header, '')}\n#include <{header}>\n")
            srcs.append(str(src))
        p = subprocess.run([str(tidy), "-fix", "-fix-errors", f"-checks=-*,{checks}", "-header-filter=.*",
                            f"--config-file={root}/libcxx/.clang-tidy", f"--load={plugin}"] + srcs + ["--"] + FLAGS + ["-I", str(cfgdir)],
                           capture_output=True, text=True)
        print(p.stdout[-2000:], p.stderr[-1500:])

UPSTREAM_BASE = "ca7933e47d3a"  # last upstream libc++ commit the headers were merged from

def added_lines(path):
    """Line numbers of `path` (relative to libcxx/include) that were added or changed since the upstream base,
    or None if the file is new."""
    full = f"libcxx/include/{path}"
    if subprocess.run(["git", "cat-file", "-e", f"{UPSTREAM_BASE}:{full}"], cwd=root, capture_output=True).returncode:
        return None
    out = subprocess.run(["git", "diff", "-U0", UPSTREAM_BASE, "--", full], cwd=root, capture_output=True, text=True).stdout
    lines = set()
    for m in re.finditer(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@", out, re.M):
        start, n = int(m[1]), int(m[2]) if m[2] is not None else 1
        lines.update(range(start, start + n))
    return lines

def fork_only(diags):
    """Keep the diagnostics on lines the fork added (the others are present in unmodified upstream code, where
    this clang-tidy version reports them too: the headers come from a newer libc++ than the clang-tidy)."""
    cache, kept = {}, []
    for d in diags:
        f, line = d.split(":")[0], int(d.split(":")[1])
        if f not in cache:
            cache[f] = added_lines(f)
        if cache[f] is None or line in cache[f]:
            kept.append(d)
    return kept

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=12)
    ap.add_argument("--header", action="append")
    ap.add_argument("--out")
    ap.add_argument("--fork-only", action="store_true", help="only diagnostics on lines the fork added or changed since the upstream base")
    ap.add_argument("--fix", help="comma separated checks whose fix-its are applied to the headers (one clang-tidy process)")
    a = ap.parse_args()
    headers = a.header or [str(h) for h in public_headers if "c++26" not in lit_header_restrictions.get(h, "")]
    if a.fix:
        return fix(headers, a.fix)
    seen = set()
    with tempfile.TemporaryDirectory() as tmp, concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
        for res in ex.map(lambda h: run(h, tmp), headers):
            seen.update(res)
    lines = sorted(seen)
    if a.fork_only:
        lines = fork_only(lines)
    text = "\n".join(lines) + ("\n" if lines else "")
    if a.out:
        pathlib.Path(a.out).write_text(text)
    else:
        sys.stdout.write(text)
    print(f"{len(lines)} diagnostics in {len({l.split(':')[0] for l in lines})} files", file=sys.stderr)

main()
