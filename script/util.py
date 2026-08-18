#!/usr/bin/env python3
# coding=utf-8

import os
import platform
import shutil
import subprocess

# "linux", "darwin_x86", "darwin_arm64", "windows"
SYSTEM_NAME = ""

def set_system_name():
    global SYSTEM_NAME
    _env = platform.system().lower()
    if "linux" in _env:
        SYSTEM_NAME = "linux"
    elif "darwin" in _env:
        machine = "x86" if "x86" in platform.machine().lower() else "arm64"
        SYSTEM_NAME = f"darwin_{machine}"
    else:
        SYSTEM_NAME = "windows"
    return SYSTEM_NAME


def get_system_name():
    global SYSTEM_NAME
    if len(SYSTEM_NAME):
        return SYSTEM_NAME
    return set_system_name()


def rm_rf(file_path):
    if os.path.isfile(file_path):
        os.remove(file_path)
    elif os.path.isdir(file_path):
        shutil.rmtree(file_path)
    return True


def copy_file(source, target, force=True) -> bool:
    '''
    force: Overwrite if the target file exists
    '''
    if not os.path.exists(source):
        print(f"Not found [{source}].")
        return False
    if not force and os.path.exists(target):
        return True

    target_dir = os.path.dirname(target)
    if target_dir:
        os.makedirs(target_dir, exist_ok=True)
    shutil.copy(source, target)
    return True


def do_subprocess(cmd: str) -> int:
    '''
    return: 0: success, other: error
    '''
    if not cmd:
        print("Subprocess cmd is empty.")
        return 0

    print(f"do subprocess: {cmd}")

    ret = 1  # 0: success
    try:
        ret = os.system(cmd)
    except Exception as e:
        print(f"Do subprocess error: {str(e)}")
        print(f"do subprocess: {cmd}")
        return 1
    return ret



def do_subprocess_argv(argv, cwd=None) -> int:
    '''
    Run a command given as an argument list, without going through a shell.

    do_subprocess() runs os.system(), which on Windows hands the string to
    cmd.exe. When the line both starts with a quote and contains more quotes,
    cmd.exe strips the outermost pair, so

        "C:\\...\\python.exe" -c "import jinja2"

    arrives split in the wrong place and fails. Passing the arguments as a
    list avoids quoting rules entirely, on every OS.

    cwd runs the command in that directory, which is what a caller wanting
    `cd <dir> && <cmd>` needs -- and unlike the shell form it stays correct
    when the directory contains a space, as a Windows checkout often does.

    return: 0: success, other: error
    '''
    if not argv:
        print("Subprocess argv is empty.")
        return 0

    prefix = f"[{cwd}] " if cwd else ""
    print("do subprocess: " + prefix + " ".join(str(a) for a in argv))

    try:
        return subprocess.run([str(a) for a in argv], cwd=cwd).returncode
    except OSError as e:
        print(f"Do subprocess error: {str(e)}")
        return 1


def find_bash() -> str:
    '''
    Locate a POSIX bash to run the script/ helpers with.

    Windows has no bash of its own; Git for Windows ships one. Note that
    C:\\Windows\\System32\\bash.exe is the WSL launcher, not Git Bash -- it sees
    a different filesystem, so it is skipped deliberately.

    return: path to bash, or "" when none was found
    '''
    if get_system_name() != "windows":
        return shutil.which("bash") or ""

    candidate = shutil.which("bash")
    if candidate and "system32" not in candidate.lower():
        return candidate

    # Derive the Git install root from git.exe, then try the usual layouts.
    roots = []
    git_exe = shutil.which("git")
    if git_exe:
        # <root>\cmd\git.exe or <root>\bin\git.exe
        roots.append(os.path.dirname(os.path.dirname(git_exe)))
    roots += [r"C:\Program Files\Git", r"C:\Program Files (x86)\Git"]

    for root in roots:
        for rel in (r"bin\bash.exe", r"usr\bin\bash.exe"):
            path = os.path.join(root, rel)
            if os.path.isfile(path):
                return path
    return ""


def run_shell_script(script, *args) -> int:
    '''
    Run one of the script/ bash helpers.

    Executing "./script/bootstrap" directly only works where the kernel honours
    the shebang. On Windows cmd.exe just reports that "." is not a command, so
    the script has to be handed to bash explicitly.

    return: 0: success, other: error
    '''
    argv = [script, *args]

    if get_system_name() == "windows":
        bash = find_bash()
        if not bash:
            print(" ** ERROR: no bash found to run " + script)
            print("    Install Git for Windows (it ships bash) and retry.")
            return 1
        argv = [bash, *argv]

    return do_subprocess_argv(argv)


def need_settarget(target_file, target):
    if not os.path.exists(target_file):
        return True
    with open(target_file, "r", encoding='utf-8') as f:
        old_target = f.read().strip()
    print(f"old_target: {old_target}")
    if target != old_target:
        return True
    return False


def record_target(target_file, target):
    with open(target_file, "w", encoding='utf-8') as f:
        f.write(target)
    return True
