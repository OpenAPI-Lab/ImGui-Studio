import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
from zipfile import ZipFile


SOURCE_ROOT = Path(__file__).resolve().parents[1]


def run(*args, env=None, cwd=None, capture=False, timeout=30):
    return subprocess.run(
        args, check=True, env=env, cwd=cwd, text=True, timeout=timeout,
        stdout=subprocess.PIPE if capture else None,
    )


def check_sha256(artifact):
    expected = artifact.with_name(artifact.name + ".sha256").read_text().split()[0]
    digest = hashlib.sha256()
    with artifact.open("rb") as source:
        for block in iter(lambda: source.read(65536), b""):
            digest.update(block)
    if digest.hexdigest() != expected.lower():
        raise RuntimeError(f"SHA-256 mismatch for {artifact.name}")


def check_licenses(doc_root, project_notice):
    source_license = SOURCE_ROOT / "LICENSE"
    if (doc_root / project_notice).read_text() != source_license.read_text():
        raise RuntimeError("The packaged project license differs from the repository license.")
    third_party = ("Dear-ImGui.txt", "GLFW.md", "imgui-knobs.txt", "nlohmann-json.txt", "stb.txt")
    if any(not (doc_root / "third-party" / name).is_file() for name in third_party):
        raise RuntimeError("The package is missing dependency notices.")


def check_documentation(doc_root):
    entry_points = (
        "README.md", "README.en.md", "CHANGELOG.md", "CHANGELOG.en.md",
        "CONTRIBUTING.md", "CONTRIBUTING.en.md", "SECURITY.md", "SECURITY.en.md",
        "include/imguistudio/localization/studio_locale_strings.inc", "examples/barra-y-lienzo-libre.json",
        "examples/tarjeta-de-proceso.widget.json", "examples/estado-y-progreso.json",
        "examples/barra-de-titulo.json", "examples/vulkan-imagenes.json",
    )
    files = [SOURCE_ROOT / name for name in entry_points]
    files.extend((SOURCE_ROOT / "docs").rglob("*.md"))
    files.extend(path for path in (SOURCE_ROOT / "examples/title-bar-action").iterdir()
                 if path.suffix in (".md", ".hpp"))
    for source in sorted(files):
        relative = source.relative_to(SOURCE_ROOT)
        installed = doc_root / relative
        if not installed.is_file() or installed.read_bytes() != source.read_bytes():
            raise RuntimeError(f"Packaged documentation is missing or outdated: {relative.as_posix()}")
    print(f"Verified {len(files)} installed documentation and example files.")


def start_editor(executable, env=None):
    result = run(str(executable), "--smoke-test", env=env, cwd=executable.parent, capture=True)
    print(result.stdout, end="")
    if "Native GLFW/OpenGL startup and three editor frames" not in result.stdout:
        raise RuntimeError("The packaged editor did not report a successful graphical smoke test.")


def check_windows_gui(executable):
    # The Subsystem field has the same offset in PE32 and PE32+.
    with executable.open("rb") as image:
        if image.read(2) != b"MZ":
            raise RuntimeError("Expected a Windows executable.")
        image.seek(0x3C)
        pe_offset = int.from_bytes(image.read(4), "little")
        image.seek(pe_offset)
        if image.read(4) != b"PE\0\0":
            raise RuntimeError("Invalid PE header.")
        image.seek(pe_offset + 24 + 68)
        if int.from_bytes(image.read(2), "little") != 2:
            raise RuntimeError("The editor must use the Windows GUI subsystem.")


def verify_zip(artifact, workspace):
    stage = workspace / "zip"
    stage.mkdir()
    with ZipFile(artifact) as archive:
        archive.extractall(stage)
    candidates = list(stage.rglob("imgui_studio.exe"))
    if len(candidates) != 1:
        raise RuntimeError("Expected one Windows editor executable.")
    executable = candidates[0]
    doc_root = executable.parent.parent / "share/doc/ImGuiStudioNative"
    check_licenses(doc_root, "LICENSE.txt")
    check_documentation(doc_root)
    if (executable.parent / "libidn2-0.dll").is_file():
        idn_notices = doc_root / "third-party/libidn2"
        if not idn_notices.is_dir() or not any(path.is_file() for path in idn_notices.rglob("*")):
            raise RuntimeError("The Windows package is missing the libidn2 notices.")
    check_windows_gui(executable)
    windows_dlls = {"kernel32.dll", "user32.dll", "opengl32.dll", "advapi32.dll"}
    if any(dll.name.lower() in windows_dlls for dll in executable.parent.glob("*.dll")):
        raise RuntimeError("The archive includes Windows system DLLs.")
    env = os.environ.copy()
    system_root = Path(os.environ["SystemRoot"])
    env["PATH"] = os.pathsep.join(map(str, (system_root / "System32", system_root)))
    start_editor(executable, env=env)


def verify_deb(artifact, workspace, dpkg, dpkg_deb, fakeroot, require_libsecret):
    package = run(dpkg_deb, "-f", str(artifact), "Package", capture=True).stdout.strip()
    if package != "imgui-studio":
        raise RuntimeError(f"Unexpected Debian package: {package}")
    if require_libsecret:
        depends = run(dpkg_deb, "-f", str(artifact), "Depends", capture=True).stdout.strip()
        if "libsecret-1-0 (>= 0.19.1)" not in map(str.strip, depends.split(",")):
            raise RuntimeError("The DEB with Secret Service support must depend on libsecret-1-0 (>= 0.19.1).")
    control = workspace / "control"
    run(dpkg_deb, "-e", str(artifact), str(control))
    for script in ("preinst", "postinst", "prerm", "postrm", "config", "triggers"):
        if (control / script).exists():
            raise RuntimeError(f"The isolated package test cannot run maintainer scripts: {script}")
    root = workspace / "deb"
    database = root / "var/lib/dpkg"
    database.mkdir(parents=True)
    options = (
        fakeroot, dpkg, f"--root={root}", f"--admindir={database}",
        f"--log={workspace / 'dpkg.log'}", "--force-not-root", "--no-triggers",
    )
    run(*options, "--force-depends", "--install", str(artifact))
    payload = (
        root / "usr/bin/imgui_studio",
        root / "usr/share/applications/imgui-studio.desktop",
        root / "usr/share/icons/hicolor/256x256/apps/imgui-studio.png",
    )
    if not all(file.is_file() for file in payload):
        raise RuntimeError("The installed package is missing application files.")
    doc_root = root / "usr/share/doc/imgui-studio"
    check_licenses(doc_root, "copyright")
    check_documentation(doc_root)
    start_editor(payload[0])
    run(*options, "--remove", package)
    if any(file.exists() for file in payload):
        raise RuntimeError("The package did not remove all application files.")
    if doc_root.exists() and any(path.is_file() for path in doc_root.rglob("*")):
        raise RuntimeError("The package did not remove all documentation files.")


def main():
    parser = argparse.ArgumentParser()
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--build", type=Path)
    source.add_argument("--artifact", type=Path, help="Verify an existing package and its .sha256 file.")
    parser.add_argument("--cpack")
    parser.add_argument("--config", default="")
    parser.add_argument("--format", choices=("ZIP", "DEB"), required=True)
    parser.add_argument("--dpkg")
    parser.add_argument("--dpkg-deb")
    parser.add_argument("--fakeroot")
    parser.add_argument("--require-libsecret", action="store_true", help="Check the DEB dependency for Secret Service.")
    args = parser.parse_args()
    extension = f".{args.format.lower()}"
    if args.build and not args.cpack:
        parser.error("--build requires --cpack.")
    if args.format == "DEB" and not all((args.dpkg, args.dpkg_deb, args.fakeroot)):
        parser.error("DEB verification requires --dpkg, --dpkg-deb and --fakeroot.")
    if args.require_libsecret and args.format != "DEB":
        parser.error("--require-libsecret requires --format DEB.")

    artifact = args.artifact.resolve() if args.artifact else None
    if artifact and (not artifact.is_file() or artifact.suffix.lower() != extension):
        parser.error(f"--artifact must name an existing {extension} file.")
    workspace_parent = artifact.parent if artifact else args.build.resolve()
    with tempfile.TemporaryDirectory(prefix="package-test-", dir=workspace_parent) as path:
        workspace = Path(path)
        if artifact is None:
            command = [args.cpack, "--config", str(workspace_parent / "CPackConfig.cmake"),
                       "-G", args.format, "-B", str(workspace)]
            if args.config:
                command.extend(("-C", args.config))
            run(*command, timeout=90)
            artifacts = list(workspace.glob(f"*{extension}"))
            if len(artifacts) != 1:
                raise RuntimeError(f"Expected one {args.format} artifact, found {len(artifacts)}.")
            artifact = artifacts[0]
        check_sha256(artifact)
        if args.format == "ZIP":
            verify_zip(artifact, workspace)
        else:
            verify_deb(artifact, workspace, args.dpkg, args.dpkg_deb, args.fakeroot, args.require_libsecret)


if __name__ == "__main__":
    main()
