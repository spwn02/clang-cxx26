#!/usr/bin/env python3
"""Stress clangd completion and clang cc1 completion at macro token boundaries."""
import argparse
import json
import os
from pathlib import Path
import select
import subprocess
import sys
import tempfile
import time
import urllib.parse

HERE = Path(__file__).resolve().parent
CASES = HERE / "cases"
INPUTS = HERE.parents[2] / "clang" / "test" / "Reflection" / "Inputs"
MACRO_CASES = (
    "expression-raw", "expression-typed", "statement-raw", "declaration-raw",
    "nested", "statement-typed", "declaration-typed", "meminit-raw",
    "meminit-typed", "member-explicit-object", "call-injected",
    "auto-decl-injected", "block-end-injected", "template",
)
CONTROLS = (
    "control-deducing-this-inlay", "control-lambda-trailing",
    "control-raw-first-token", "control-meminit",
)
INLAY_CASE = "control-deducing-this-inlay"
IGNORE_LSP_ERRORS = {-32001, -32602}


def load_case(name):
    raw = (CASES / (name + ".cpp")).read_text()
    cursor = raw.index("/*CURSOR*/") if "/*CURSOR*/" in raw else None
    code = raw.replace("/*CURSOR*/", "")
    return code, cursor


def cursor_position(code, offset):
    before = code[:offset]
    line = before.count("\n")
    col = len(before.rsplit("\n", 1)[-1])
    return line, col


def token_spans(code):
    """Return every character boundary in invocations and quote bodies."""
    found = {}
    # An invocation begins at its macro identifier and ends at the matching )
    # plus an optional semicolon. Nested invocations are independently found.
    for bang in (i for i, c in enumerate(code) if c == "!"):
        if bang + 1 >= len(code) or code[bang + 1] != "(":
            continue
        start = bang - 1
        while start >= 0 and (code[start].isalnum() or code[start] == "_"):
            start -= 1
        start += 1
        depth = 0
        end = None
        for j in range(bang + 1, len(code)):
            if code[j] == "(":
                depth += 1
            elif code[j] == ")":
                depth -= 1
                if depth == 0:
                    end = j + 1
                    break
        if end is None:
            continue
        semi = end
        while semi < len(code) and code[semi] in " \t":
            semi += 1
        if semi < len(code) and code[semi] == ";":
            end = semi + 1
        found[(start, end, "invocation")] = True

    # The quote-body positions include tokens after the opener, every character
    # inside holes, and the boundary immediately before the closing brace.
    pos = 0
    while True:
        opener = code.find("^{", pos)
        if opener < 0:
            break
        brace = opener + 2
        depth = 1
        i = brace + 1
        while i < len(code) and depth:
            if code[i] == "{":
                depth += 1
            elif code[i] == "}":
                depth -= 1
            i += 1
        if depth == 0:
            found[(brace + 1, i - 1, "quote-body")] = True
            pos = i
        else:
            break

    return found


def positions_for(name, code, marker):
    locations = []
    for start, end, kind in token_spans(code):
        for offset in range(start, end + 1):
            line, col = cursor_position(code, offset)
            locations.append({"line": line, "character": col, "offset": offset,
                              "class": kind})
    if marker is not None:
        line, col = cursor_position(code, marker)
        kind = {
            "control-lambda-trailing": "lambda-trailing",
            "control-raw-first-token": "raw-first-token",
            "control-meminit": "mem-initializer",
        }.get(name, "control")
        locations.append({"line": line, "character": col, "offset": marker,
                          "class": kind})
    # Stable unique positions, preserving the most specific class for overlaps.
    unique = {}
    for loc in locations:
        key = (loc["line"], loc["character"])
        if (key not in unique or loc["class"] == "quote-body"
                or loc["class"] not in ("invocation", "quote-body")):
            unique[key] = loc
    return list(unique.values())


class LSP:
    def __init__(self, binary, root, path, code, timeout):
        self.stderr_file = tempfile.TemporaryFile()
        self.proc = subprocess.Popen(
            [binary, "--enable-config", "--compile-commands-dir=" + str(root),
             "--background-index=0", "--clang-tidy=0", "--log=error"],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=self.stderr_file, bufsize=0)
        self.path = path
        self.uri = "file:" + urllib.parse.quote(str(path))
        self.timeout = timeout
        self.next_id = 0

    def send(self, obj):
        body = json.dumps(obj, separators=(",", ":")).encode()
        self.proc.stdin.write(b"Content-Length: " + str(len(body)).encode() +
                              b"\r\n\r\n" + body)

    def receive(self, wanted):
        until = time.monotonic() + self.timeout
        stream = self.proc.stdout
        header = bytearray()
        while time.monotonic() < until:
            if self.proc.poll() is not None:
                raise RuntimeError("clangd exited before response (rc=%s)" % self.proc.returncode)
            ready, _, _ = select.select([stream], [], [], max(0, until-time.monotonic()))
            if not ready:
                raise TimeoutError("clangd response timeout")
            byte = stream.read(1)
            if not byte:
                raise RuntimeError("clangd closed stdout before response")
            header.extend(byte)
            if header.endswith(b"\r\n\r\n"):
                break
        else:
            raise TimeoutError("clangd response timeout reading header")
        try:
            length = int(next(x.split(b":", 1)[1] for x in header.split(b"\r\n")
                              if x.lower().startswith(b"content-length:")))
            body = bytearray()
            while len(body) < length:
                if self.proc.poll() is not None:
                    raise ValueError("clangd exited during JSON-RPC body")
                ready, _, _ = select.select([stream], [], [],
                                            max(0, until-time.monotonic()))
                if not ready:
                    raise TimeoutError("timeout reading JSON-RPC body")
                chunk = stream.read(length - len(body))
                if not chunk:
                    raise ValueError("truncated JSON-RPC body")
                body.extend(chunk)
            response = json.loads(body)
        except Exception as exc:
            raise ValueError("malformed JSON-RPC response: %s" % exc) from exc
        if response.get("id") == wanted:
            return response
        return self.receive(wanted)

    def request(self, method, params):
        self.next_id += 1
        ident = self.next_id
        self.send({"jsonrpc": "2.0", "id": ident, "method": method,
                   "params": params})
        return self.receive(ident)

    def close(self):
        """Shut clangd down; report slow or hung shutdowns instead of hiding them."""
        phase = "shutdown-request"
        start = time.monotonic()
        self.shutdown_note = None
        try:
            if self.proc.poll() is None:
                self.send({"jsonrpc": "2.0", "id": 999999, "method": "shutdown",
                           "params": {}})
                self.receive(999999)
                phase = "exit-wait"
                self.send({"jsonrpc": "2.0", "method": "exit", "params": {}})
                self.proc.wait(timeout=EXIT_WAIT_SECONDS)
            elapsed = time.monotonic() - start
            if elapsed > 3:
                self.shutdown_note = "slow shutdown %.1fs" % elapsed
        except Exception as error:
            self.shutdown_note = "hung in %s after %.1fs (%r)" % (
                phase, time.monotonic() - start, error)
            subprocess.run(["eu-stack", "-p", str(self.proc.pid)],
                           stdout=sys.stderr, stderr=subprocess.DEVNULL,
                           timeout=60, check=False)
            self.proc.kill()
            self.proc.wait()
        self.stderr_file.seek(0)
        stderr = self.stderr_file.read().decode(errors="replace")
        self.stderr_file.close()
        return self.proc.returncode, stderr


EXIT_WAIT_SECONDS = 30  # generous: a kill after this is a real hang, not load


def run_lsp(binary, repeats, timeout, only_cases=None):
    results = []
    cases = MACRO_CASES + CONTROLS
    if only_cases:
        cases = [name for name in cases if name in only_cases]
    with tempfile.TemporaryDirectory(prefix="clangd-stress-") as temp:
        root = Path(temp)
        (root / ".clangd").write_text(
            "InlayHints:\n  Enabled: true\n  ParameterNames: true\n"
            "  DeducedTypes: true\n  BlockEnd: true\n")
        for name in cases:
            code, marker = load_case(name)
            path = root / (name + ".cpp")
            path.write_text(code)
            (root / "compile_commands.json").write_text(json.dumps([{
                "directory": str(root), "file": str(path),
                "arguments": ["clang++", "-std=c++29", "-fsyntax-only",
                              "-I", str(INPUTS), str(path)],
            }]))
            locations = positions_for(name, code, marker)
            for repeat in range(1, repeats + 1):
                failures = []
                sent = 0
                client = None
                try:
                    client = LSP(binary, root, path, code, timeout)
                    init = client.request("initialize", {
                        "processId": None, "rootUri": root.as_uri(),
                        "capabilities": {"textDocument": {
                            "completion": {"completionItem": {"snippetSupport": False}},
                            "inlayHint": {"dynamicRegistration": False},
                        }},
                    })
                    if "error" in init:
                        raise RuntimeError("initialize failed: " + str(init["error"]))
                    client.send({"jsonrpc": "2.0", "method": "initialized", "params": {}})
                    client.send({"jsonrpc": "2.0", "method": "textDocument/didOpen",
                                 "params": {"textDocument": {"uri": client.uri,
                                 "languageId": "cpp", "version": 1, "text": code}}})
                    time.sleep(0.25)
                    for loc in locations:
                        try:
                            sent += 1
                            response = client.request("textDocument/completion", {
                                "textDocument": {"uri": client.uri},
                                "position": {"line": loc["line"],
                                             "character": loc["character"]},
                                "context": {"triggerKind": 1},
                            })
                            if "error" in response and response["error"].get("code") not in IGNORE_LSP_ERRORS:
                                failures.append({"position": loc, "error": response["error"]})
                            if (name in ("control-raw-first-token", "control-meminit")
                                    and loc.get("class") in ("raw-first-token", "mem-initializer")
                                    and "partial_value" not in json.dumps(response.get("result"))):
                                failures.append({"position": loc,
                                                 "failure": "completion omitted partial_value"})
                        except Exception as exc:
                            failures.append({"position": loc, "failure": str(exc)})
                            break
                    if name == INLAY_CASE and not failures:
                        # Full-document inlay request exercises the dependent-base
                        # deducing-this member-call path from #532.
                        lines = code.splitlines()
                        try:
                            response = client.request("textDocument/inlayHint", {
                                "textDocument": {"uri": client.uri},
                                "range": {"start": {"line": 0, "character": 0},
                                          "end": {"line": len(lines) - 1,
                                                  "character": len(lines[-1])}},
                            })
                            if "error" in response:
                                failures.append({"inlay-error": response["error"]})
                            elif "a:" not in json.dumps(response.get("result")):
                                failures.append({"inlay-failure": "missing dependent call hint a:",
                                                 "result": response.get("result")})
                        except Exception as exc:
                            failures.append({"inlay-failure": str(exc)})
                except Exception as exc:
                    failures.append({"setup-failure": str(exc)})
                finally:
                    if client:
                        rc, stderr = client.close()
                        if rc not in (0, -15):
                            failures.append({"exit": rc, "stderr": stderr[-1500:],
                                             "shutdown": client.shutdown_note})
                        elif client.shutdown_note:
                            print(json.dumps({"case": name, "repeat": repeat,
                                              "note": client.shutdown_note}),
                                  flush=True)
                results.append({"case": name, "repeat": repeat,
                                "positions": len(locations),
                                "completion_requests": sent,
                                "failures": failures})
                print(json.dumps(results[-1]), flush=True)
    bad = sum(bool(r["failures"]) for r in results)
    print(json.dumps({"mode": "lsp", "cases": len(cases), "runs": len(results),
                      "completion_requests": sum(r["completion_requests"] for r in results),
                      "failed_runs": bad}), flush=True)
    return int(bool(bad))


def compiler_classes():
    classes = {}
    for name in MACRO_CASES + CONTROLS:
        code, marker = load_case(name)
        if name not in MACRO_CASES and marker is None:
            continue
        groups = {}
        if name in MACRO_CASES:
            for loc in positions_for(name, code, marker):
                groups.setdefault(loc["class"], []).append(loc)
        elif marker is not None:
            line, col = cursor_position(code, marker)
            kind = {
                "control-lambda-trailing": "lambda-trailing",
                "control-raw-first-token": "raw-first-token",
                "control-meminit": "mem-initializer",
            }.get(name, "control")
            loc = {"line": line, "character": col, "offset": marker,
                   "class": kind}
            groups.setdefault(loc["class"], []).append(loc)
        for kind, locations in groups.items():
            classes.setdefault(kind, []).append((name, code, locations))
    return classes


def run_cc1(binary, repeats):
    classes = compiler_classes()
    total = failures = 0
    for kind, cases in classes.items():
        positions = [(name, code, loc) for name, code, locs in cases for loc in locs]
        class_failures = []
        for name, code, loc in positions:
            for iteration in range(1, repeats + 1):
                with tempfile.TemporaryDirectory(prefix="clang-cc1-stress-") as temp:
                    source = Path(temp) / (name + ".cpp")
                    source.write_text(code)
                    line = loc["line"] + 1
                    column = loc["character"] + 1
                    command = [binary, "-cc1", "-std=c++29", "-I", str(INPUTS),
                               "-fsyntax-only", "-code-completion-at=%s:%d:%d" %
                               (source, line, column), str(source), "-o", "-"]
                    try:
                        run = subprocess.run(command, capture_output=True, text=True,
                                             timeout=30)
                        total += 1
                        output = run.stdout + run.stderr
                        if run.returncode >= 128 or "Assertion" in output:
                            class_failures.append({"case": name,
                                                   "position": [line, column],
                                                   "iteration": iteration,
                                                   "returncode": run.returncode,
                                                   "tail": output[-1200:]})
                        elif kind in ("raw-first-token", "mem-initializer") and \
                                "COMPLETION: partial_value" not in output:
                            class_failures.append({"case": name,
                                                   "position": [line, column],
                                                   "iteration": iteration,
                                                   "failure": "completion omitted partial_value",
                                                   "tail": output[-1200:]})
                        elif kind == "raw-first-token" and \
                                "COMPLETION: Pattern : return" in output:
                            class_failures.append({"case": name,
                                                   "position": [line, column],
                                                   "iteration": iteration,
                                                   "failure": "statement completion leaked into raw expression",
                                                   "tail": output[-1200:]})
                    except subprocess.TimeoutExpired:
                        total += 1
                        class_failures.append({"case": name,
                                               "position": [line, column],
                                               "iteration": iteration,
                                               "failure": "timeout"})
        failures += bool(class_failures)
        print(json.dumps({"class": kind, "positions": len(positions),
                          "runs": repeats * len(positions),
                          "failures": class_failures}), flush=True)
    print(json.dumps({"mode": "cc1", "classes": len(classes), "runs": total,
                      "failed_classes": failures}), flush=True)
    return int(bool(failures))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("clangd", help="clangd executable to probe")
    parser.add_argument("--clang", help="matching clang executable for -cc1 probes")
    parser.add_argument("--mode", choices=("lsp", "cc1", "all"), default="all")
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--cc1-repeats", type=int, default=25)
    parser.add_argument("--timeout", type=float, default=12)
    parser.add_argument("--case", action="append", help="limit LSP probe to named case")
    args = parser.parse_args()
    rc = 0
    if args.mode in ("lsp", "all"):
        rc |= run_lsp(args.clangd, args.repeats, args.timeout, args.case)
    if args.mode in ("cc1", "all"):
        if not args.clang:
            parser.error("--clang is required for cc1 and all modes")
        rc |= run_cc1(args.clang, args.cc1_repeats)
    return rc


if __name__ == "__main__":
    sys.exit(main())
