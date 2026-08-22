#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from typing import NoReturn


RELEASE_ID = re.compile(r"^p2996-\d{4}\.\d{2}\.\d{2}(?:\.\d+)?$")
DEV_ID = re.compile(r"^p2996-dev-[0-9a-f]{7,40}$")


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def digest(path: Path, algorithm: str) -> str:
    value = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def first_line(command: list[str]) -> str:
    result = subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return result.stdout.splitlines()[0]


def discover_library_dirs(stage: Path) -> list[Path]:
    parents: set[Path] = set()

    for pattern in (
        "libc++.so*",
        "libc++.a",
        "libc++abi.so*",
        "libc++abi.a",
        "libunwind.so*",
        "libunwind.a",
    ):
        for item in stage.rglob(pattern):
            if item.is_file() or item.is_symlink():
                parents.add(item.parent.relative_to(stage))

    if not parents:
        fail("no libc++/libc++abi/libunwind libraries were found in the staged installation")

    return sorted(parents)


def render_template(source: Path, replacements: dict[str, str]) -> str:
    text = source.read_text(encoding="utf-8")
    for key, value in replacements.items():
        text = text.replace(f"@{key}@", value)
    return text


def create_archive(package_parent: Path, root_name: str, archive: Path, source_date_epoch: int) -> None:
    tar = subprocess.Popen(
        [
            "tar",
            "--sort=name",
            f"--mtime=@{source_date_epoch}",
            "--owner=0",
            "--group=0",
            "--numeric-owner",
            "--pax-option=delete=atime,delete=ctime",
            "-C",
            str(package_parent),
            "-cf",
            "-",
            root_name,
        ],
        stdout=subprocess.PIPE,
    )

    assert tar.stdout is not None
    try:
        subprocess.run(
            ["zstd", "-19", "-T0", "-f", "-o", str(archive)],
            stdin=tar.stdout,
            check=True,
        )
    finally:
        tar.stdout.close()

    if tar.wait() != 0:
        fail("tar failed while creating the reference-toolchain archive")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--snapshot", required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--source-date-epoch", required=True, type=int)
    args = parser.parse_args()

    stage = args.stage.resolve()
    output = args.output.resolve()

    if not RELEASE_ID.fullmatch(args.snapshot) and not DEV_ID.fullmatch(args.snapshot):
        fail(
            "snapshot id must be p2996-YYYY.MM.DD[.N] or "
            "p2996-dev-<7..40 lowercase hex characters>"
        )

    if not re.fullmatch(r"[0-9a-f]{40}", args.revision):
        fail("--revision must be a full lowercase 40-character Git commit id")

    required = [
        stage / "bin" / "clang",
        stage / "bin" / "clang++",
        stage / "bin" / "clang-scan-deps",
    ]
    for item in required:
        if not item.exists():
            fail(f"staged installation is missing {item.relative_to(stage)}")

    module_manifests = sorted(stage.rglob("libc++.modules.json"))
    if len(module_manifests) != 1:
        fail(
            "expected exactly one installed libc++.modules.json, found "
            f"{len(module_manifests)}"
        )

    module_manifest = module_manifests[0].relative_to(stage)
    library_dirs = discover_library_dirs(stage)
    clang_version = first_line([str(stage / "bin" / "clang++"), "--version"])

    output.mkdir(parents=True, exist_ok=True)

    asset_stem = f"clang-{args.snapshot}-linux-x86_64"
    archive = output / f"{asset_stem}.tar.zst"
    external_manifest = output / f"{asset_stem}.manifest.json"
    sha256_file = output / f"{asset_stem}.SHA256SUMS"
    sha512_file = output / f"{asset_stem}.SHA512SUMS"
    root_name = f"clang-{args.snapshot}"

    with tempfile.TemporaryDirectory(prefix="clang-p2996-package-") as temporary:
        package_parent = Path(temporary)
        package_root = package_parent / root_name
        shutil.copytree(stage, package_root, symlinks=True)

        metadata_dir = package_root / "share" / "clang-p2996"
        metadata_dir.mkdir(parents=True, exist_ok=True)

        cmake_rpath = ";".join(
            f"${{_p2996_root}}/{path.as_posix()}" for path in library_dirs
        )
        library_path = ":".join(
            f"${{P2996_TOOLCHAIN_ROOT}}/{path.as_posix()}" for path in library_dirs
        )

        template_dir = Path(__file__).resolve().parent

        toolchain = render_template(
            template_dir / "toolchain.cmake.in",
            {"P2996_BUILD_RPATH": cmake_rpath},
        )
        (metadata_dir / "toolchain.cmake").write_text(toolchain, encoding="utf-8")

        activation = render_template(
            template_dir / "activate.sh.in",
            {"P2996_LIBRARY_PATH": library_path},
        )
        activation_path = metadata_dir / "activate.sh"
        activation_path.write_text(activation, encoding="utf-8")
        activation_path.chmod(0o755)

        manifest = {
            "schemaVersion": 1,
            "snapshot": args.snapshot,
            "source": {
                "repository": "https://github.com/spwn02/clang-p2996",
                "branch": "p2996",
                "revision": args.revision,
            },
            "platform": {
                "os": "linux",
                "architecture": "x86_64",
                "target": "x86_64-unknown-linux-gnu",
            },
            "compiler": {
                "name": "clang-p2996",
                "version": clang_version,
                "reflectionMode": "-freflection-latest",
            },
            "standardLibrary": {
                "name": "libc++",
                "modulesManifest": module_manifest.as_posix(),
                "libraryDirectories": [path.as_posix() for path in library_dirs],
            },
            "components": [
                "clang",
                "clang-scan-deps",
                "lld",
                "llvm-ar",
                "libc++",
                "libc++abi",
                "libunwind",
                "libc++ standard-module sources",
            ],
            "cmake": {
                "minimumConsumerVersion": "4.4",
                "toolchainFile": "share/clang-p2996/toolchain.cmake",
            },
            "activationScript": "share/clang-p2996/activate.sh",
            "sourceDateEpoch": args.source_date_epoch,
        }

        manifest_text = json.dumps(manifest, indent=2, sort_keys=True) + "\n"
        (metadata_dir / "manifest.json").write_text(manifest_text, encoding="utf-8")
        external_manifest.write_text(manifest_text, encoding="utf-8")

        create_archive(package_parent, root_name, archive, args.source_date_epoch)

    entries = [archive, external_manifest]

    sha256_file.write_text(
        "".join(f"{digest(path, 'sha256')}  {path.name}\n" for path in entries),
        encoding="utf-8",
    )
    sha512_file.write_text(
        "".join(f"{digest(path, 'sha512')}  {path.name}\n" for path in entries),
        encoding="utf-8",
    )

    print(archive)
    print(external_manifest)
    print(sha256_file)
    print(sha512_file)
    return 0


if __name__ == "__main__":
    sys.exit(main())
