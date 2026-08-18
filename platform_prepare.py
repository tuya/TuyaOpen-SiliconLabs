#!/usr/bin/env python3
# coding=utf-8

import sys
import os
import json
import subprocess

from script.util import (
    rm_rf, copy_file,
    do_subprocess_argv, run_shell_script,
    get_system_name
)


def get_deps(root):
    dep_file = os.path.join(root, "platform_libsdepend")
    if not os.path.exists(dep_file):
        print("Dependency file not found.")
        return []
    with open(dep_file, "r", encoding="utf-8") as f:
        try:
            deps = json.load(f)
        except Exception as e:
            print(f"Failed to parse dependency file: {e}")
            return []
    return deps


def _is_git_repo(path):
    git_dir = os.path.join(path, ".git")
    return os.path.exists(git_dir)


def _run_git_capture(path, args):
    cmd = ["git", "-C", path] + args
    ret = subprocess.run(
        cmd,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if ret.returncode != 0:
        return None
    return ret.stdout.strip()


def _git_head_matches_version(path, version):
    """
    Validate current SDK checkout against expected version.
    Supports both commit-hash and tag/branch-like refs.
    """
    if not _is_git_repo(path):
        return False

    head = _run_git_capture(path, ["rev-parse", "HEAD"])
    if not head:
        return False

    # Commit hash mode
    hex_chars = set("0123456789abcdefABCDEF")
    if 7 <= len(version) <= 40 and all(c in hex_chars for c in version):
        return head.startswith(version.lower())

    # Tag/ref mode
    wanted = _run_git_capture(path, ["rev-parse", f"{version}^{{commit}}"])
    if not wanted:
        return False

    return head == wanted


def _normalize_slce_line_endings(dest_path):
    """
    Some external SDK .slce files contain CRLF and later scripts parse `id:`
    lines with shell tools. Normalize to LF to avoid IDs like `wiseconnect3_sdk\\r`.
    """
    if not os.path.isdir(dest_path):
        return

    for name in os.listdir(dest_path):
        if not name.endswith(".slce"):
            continue
        file_path = os.path.join(dest_path, name)
        try:
            with open(file_path, "rb") as f:
                content = f.read()
            normalized = content.replace(b"\r\n", b"\n")
            if normalized != content:
                with open(file_path, "wb") as f:
                    f.write(normalized)
                print(f"Normalized line endings: {file_path}")
        except Exception as e:
            print(f"Warning: Failed to normalize {file_path}: {e}")


def need_prepare(root, prepare_file, target):
    result = False
    libs_need_prepare = []
    deps = get_deps(root)
    for dep in deps:
        name = dep.get("name", "")
        path = dep.get("path", "")
        version = dep.get("version", "")
        if not name or not path or not version:
            print(f"Invalid dependency config: {dep}")
            result = True
            continue

        dest_path = os.path.join(root, path)
        if not os.path.exists(dest_path):
            print(f"{name} not exists, need prepare.")
            libs_need_prepare.append(name)
            result = True
            continue

        if not _git_head_matches_version(dest_path, version):
            print(f"{name} version mismatch, need prepare.")
            libs_need_prepare.append(name)
            result = True

    toolchain_path = os.path.join(root, "tools/toolchain")
    if not os.path.exists(toolchain_path):
        print("Tools path not exists, need prepare.")
        result = True
    if not _slc_installed(root):
        print("SLC CLI not found, need prepare.")
        result = True
    return result, libs_need_prepare


def record_prepare(prepare_file, target):
    with open(prepare_file, "a", encoding='utf-8') as f:
        f.write(target + "\n")
    return True


def _slc_installed(root):
    """
    Return True if an slc/slc-cli executable is available on PATH or under tools/slc.
    """
    import shutil
    if shutil.which("slc") or shutil.which("slc-cli"):
        return True
    slc_dir = os.path.join(root, "tools", "slc")
    if not os.path.isdir(slc_dir):
        return False
    for dirpath, _, filenames in os.walk(slc_dir):
        for name in filenames:
            if name.startswith("slc"):
                path = os.path.join(dirpath, name)
                if os.path.isfile(path) and os.access(path, os.X_OK):
                    return True
    return False


def _slc_bundled_python(root):
    """
    Locate the CPython that SLC embeds to render its Jinja templates.

    The interpreter's place differs by host -- CPython on Windows keeps
    python.exe at the top of the distribution while the Unix builds put a
    version-named binary under bin/ -- so probe instead of naming one path. A
    hardcoded bin/python3.10 silently matched nothing on Windows, and the only
    symptom was slc generate reporting "Couldn't open script file" for every
    template long afterwards.

    return: path to the interpreter, or None
    """
    import glob

    base = os.path.join(root, "tools", "slc", "slc_cli", "bin", "slc-cli",
                        "developer", "adapter_packs", "python")
    candidates = [
        os.path.join(base, "bin", "python3"),   # Unix, unversioned
    ]
    # Windows layout is unverified against a real install, so accept the
    # interpreter at either level rather than betting on one.
    candidates += sorted(glob.glob(os.path.join(base, "python*.exe")))
    candidates += sorted(glob.glob(os.path.join(base, "bin", "python*.exe")))
    # Versioned Unix names last, newest first, so a version bump still resolves.
    candidates += sorted(
        glob.glob(os.path.join(base, "bin", "python3.[0-9]*")), reverse=True)

    for path in candidates:
        # bin/ also holds python3.10-config and friends, which are not
        # interpreters.
        if "config" in os.path.basename(path):
            continue
        if os.path.isfile(path):
            return path
    return None


def _ensure_jinja2(root):
    """
    SLC template generation needs Jinja2 in BOTH:
    - the active interpreter (TuyaOpen .venv may be first on PATH)
    - SLC's bundled Python under tools/slc
    """
    pythons = []
    # Active interpreter
    pythons.append(sys.executable)
    # Common PATH names
    import shutil
    for name in ("python3", "python"):
        p = shutil.which(name)
        if p:
            pythons.append(p)
    # SLC bundled Python
    slc_py = _slc_bundled_python(root)
    if slc_py:
        pythons.append(slc_py)
    else:
        # Say so rather than skipping quietly. Without Jinja2 in this
        # interpreter, slc generate fails much later with "Couldn't open script
        # file" against each template and names nothing else.
        print("WARNING: SLC's bundled Python was not found; "
              "template generation will fail if it lacks Jinja2")

    # Unique preserve order
    seen = set()
    uniq = []
    for p in pythons:
        if p and p not in seen:
            seen.add(p)
            uniq.append(p)

    # Argument lists, not command strings: the interpreter path holds spaces on
    # Windows ("C:\\Users\\...\\.venv\\Scripts\\python.exe") and quoting it inside
    # a string that cmd.exe then re-parses breaks the command apart. See
    # do_subprocess_argv in script/util.py.
    for py in uniq:
        check = [py, "-c", "import jinja2"]
        if do_subprocess_argv(check) == 0:
            continue
        print(f"Jinja2 missing in {py}; installing ...")
        # Prefer ensurepip+pip on that exact interpreter
        do_subprocess_argv([py, "-m", "ensurepip", "--upgrade"])
        if do_subprocess_argv([py, "-m", "pip", "install", "Jinja2~=3.1.2"]) != 0:
            # Fallback for uv-managed envs without pip module
            if do_subprocess_argv(
                    ["uv", "pip", "install", "--python", py, "Jinja2~=3.1.2"]) != 0:
                print(f"ERROR: Failed to install Jinja2 for {py}")
                return False
        if do_subprocess_argv(check) != 0:
            print(f"ERROR: Jinja2 still missing for {py}")
            return False
    return True


def download_tools(root, prepare_file):
    print("Downloading Tools...")
    toolchain_path = os.path.join(root, "tools/toolchain")
    need_toolchain = not os.path.exists(toolchain_path)
    need_slc = not _slc_installed(root)

    if need_toolchain:
        print("Initializing toolchain + Silicon Labs tools ...")
        if run_shell_script("./script/bootstrap") != 0:
            return False
    elif need_slc:
        # Toolchain may already exist from a partial bootstrap; still need SLC CLI.
        print("SLC CLI not found, installing Silicon Labs tools ...")
        if run_shell_script("./script/bootstrap_silabs") != 0:
            print("Failed to install SLC CLI. Run: ./script/bootstrap silabs")
            return False

    if not _slc_installed(root):
        print("ERROR: SLC CLI still missing after bootstrap.")
        print("Install manually: cd platform/SiWx917 && ./script/bootstrap silabs")
        return False

    if not _ensure_jinja2(root):
        return False
    return True


def download_sdk(root, prepare_file, prepare_libs):
    deps = get_deps(root)
    if not deps:
        return False
    for dep in deps:
        name = dep.get("name", "")
        url = dep.get("url", "")
        version = dep.get("version", "")
        path = dep.get("path", "")
        init_submodule = dep.get("init_submodule", False)
        patch = dep.get("patch", None)
        if name not in prepare_libs:
            continue
        if not url or not version or not path:
            print(f"Invalid dependency config: {dep}")
            continue
        # Clone
        dest_path = os.path.join(root, path)
        if not os.path.exists(dest_path):
            print(f"Cloning {name}: {url} ({version}) to {dest_path}")
            cmds = [
                "git",
                "clone",
                "--recursive",
                url,
                "-b",
                version,
                "--depth=1",
                dest_path,
            ]
            if do_subprocess_argv(cmds) != 0:
                print(f"Failed to clone {name}")
                return False
        else:
            print(f"Dependency already exists: {dest_path}")
            cmds = [
                "git",
                "reset",
                "--hard",
                version
            ]
            if do_subprocess_argv(cmds, cwd=dest_path) != 0:
                print(f"Failed to checkout {version} for {name}")
                return False
        # Init submodule
        if init_submodule:
            cmds = [
                "git",
                "submodule",
                "update",
                "--init",
                "--recursive",
            ]
            if do_subprocess_argv(cmds, cwd=dest_path) != 0:
                print(f"Failed to update submodules for {name}")
                return False
        # Apply Patch
        if patch is not None:
            patch_file = os.path.join(root, patch)
            if os.path.exists(patch_file):
                cmds = [
                    "git",
                    "apply",
                    patch_file,
                ]
                if do_subprocess_argv(cmds, cwd=dest_path) != 0:
                    print(f"Failed to apply patch for {name}")
                    return False
        _normalize_slce_line_endings(dest_path)
        record_prepare(prepare_file, name)
    return True


def platform_prepare(root, target):
    prepare_file = os.path.join(root, ".prepare")
    prepare_flag, prepare_libs = need_prepare(root, prepare_file, target)

    # Always ensure Jinja2 for SLC template generation (even if SDKs already prepared).
    if not _ensure_jinja2(root):
        print("Install Jinja2 failed.")
        return False

    if not prepare_flag:
        print("No need prepare.")
        return True

    if not download_tools(root, prepare_file):
        print("Install toolchain failed.")
        return False

    if not download_sdk(root, prepare_file, prepare_libs):
        print("Download SDK failed.")
        return False

    return True


def main():
    print("================== platform_prepare ==================")
    target = "SiWx917"
    print(f"target: {target} ")
    root = os.path.dirname(os.path.abspath(__file__))
    if not platform_prepare(root, target):
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
