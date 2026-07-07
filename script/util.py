#!/usr/bin/env python3
# coding=utf-8

import os
import platform
import shutil
import requests
from git import Git

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
