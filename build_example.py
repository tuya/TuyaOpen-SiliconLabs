#!/usr/bin/env python3
# coding=utf-8
# 参数说明：
# $1 - params path: $1/build_param.[cmake/config/json]
# $2 - user cmd: build/clean/...

import os
import sys
import json
import subprocess
import shutil

def clean(root):
    shutil.rmtree("build", ignore_errors=True)
    pass

def parser_para_file(json_file):
    if not os.path.isfile(json_file):
        print(f"Error: File not found [{json_file}].")
        return {}
    try:
        with open(json_file, 'r', encoding='utf-8') as f:
            json_data = json.load(f)
    except Exception as e:
        print(f"JSON parsing error: [{str(e)}].")
        return {}
    return json_data


def check_dependency_changes(build_param_path, build_dir="build"):
    """
    检查应用库文件是否比当前可执行文件更新
    如果是，则需要强制重新构建以避免缓存问题
    """
    try:
        build_param_file = os.path.join(build_param_path, "build_param.json")
        param_data = parser_para_file(build_param_file)
        if not param_data:
            return False
        
        project_name = param_data.get('CONFIG_PROJECT_NAME', '')
        libs_dir = param_data.get('OPEN_LIBS_DIR', '')
        
        if not project_name or not libs_dir:
            return False
            
        # 检查关键库文件
        key_libs = ['libtuyaapp.a', 'libtuyaos.a']
        current_exe = os.path.join(build_dir, project_name)
        
        if not os.path.exists(current_exe):
            return True  # 可执行文件不存在，需要构建
            
        exe_mtime = os.path.getmtime(current_exe)
        
        for lib_name in key_libs:
            lib_path = os.path.join(libs_dir, lib_name)
            if os.path.exists(lib_path):
                lib_mtime = os.path.getmtime(lib_path)
                if lib_mtime > exe_mtime:
                    print(f"Detected library file {lib_name} has been updated, rebuilding to avoid cache issues")
                    return True
                    
        return False
        
    except Exception as e:
        print(f"Error occurred during dependency check: {e}")
        return True  # Rebuild on error

def main():
    if len(sys.argv) < 3:
        print(f"Error: At least 2 parameters are needed {sys.argv}.")
        sys.exit(1)
    build_param_path = os.path.realpath(sys.argv[1])
    user_cmd = sys.argv[2]
    root = os.path.realpath(os.path.dirname(os.path.abspath(__file__)))
    print("================== build_example ==================")
    print(sys.argv)
    if "clean" == user_cmd:
        clean(root)
        sys.exit(0)

    build_param_file = os.path.join(build_param_path, "build_param.json")
    param_data = parser_para_file(build_param_file)
    if not len(param_data):
        sys.exit(1)

    cmake_lists = os.path.join(root, "CMakeLists.txt")
    if not os.path.isfile(cmake_lists):
        print(f"Error: CMakeLists.txt not found in [{root}].")
        sys.exit(1)

    # Check dependency changes, clean build directory if needed
    if check_dependency_changes(build_param_path):
        print("Detected application library updates, cleaning platform build cache...")
        clean(root)

    project_name = param_data.get('CONFIG_PROJECT_NAME', '')
    build_param_root = os.path.realpath(os.path.dirname(build_param_path))
    siwx917_lib_path = os.path.join(build_param_root, f"slc/tuyaopen_{project_name}_cmake")
    siwx917_bin_path = os.path.join(build_param_root, f"slc/tuyaopen_{project_name}_build")
    project_build_path = os.path.join(build_param_root, f"{project_name}")
    toolchain_file = os.path.join(root, "toolchain_file.cmake")
    try:
        # Add all lib include dir to slc build
        open_header_dir = param_data.get('OPEN_HEADER_DIR', '')
        cflags = os.environ.get('CFLAGS', '')
        if open_header_dir:
            for inc_dir in open_header_dir.split(' '):
                cflags += f' -I{inc_dir.strip()}'
        # Add tuyaos_adapter include to slc build
        cflags += f' -I{root}/tuyaos_adapter/include/utilities/include'
        cflags += f' -I{root}/tuyaos_adapter/include/config'
        os.environ['CFLAGS'] = cflags
        cmake_command = [
            "cmake", "-S", root, "-B", f"{project_build_path}", "-G", "Ninja",
            f"-DPLATFORM_PATH={root}",
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
            f"-DBUILD_PARAM_PATH={build_param_path}",
            f"-DTUYAOPEN_PLATFORM_SIWX917=slc_tuyaopen_{project_name}",
            f"-DTUYAOPEN_PLATFORM_SIWX917_LIB_DIR={siwx917_lib_path}",
            f"-DTUYAOPEN_PLATFORM_SIWX917_BIN_DIR={siwx917_bin_path}"
        ]

        subprocess.run(
            cmake_command,
            check=True,
            cwd=root,
        )
        subprocess.run(["cmake", "--build", f"{project_build_path}"], check=True, cwd=root)
    except subprocess.CalledProcessError as e:
        print(f"CMake build process failed: {e}")
        sys.exit(1)

    out_file_elf = os.path.join(os.getcwd(), f"{project_build_path}", f"{param_data['CONFIG_PROJECT_NAME']}.elf")
    out_file_s37 = os.path.join(os.getcwd(), f"{project_build_path}", f"{param_data['CONFIG_PROJECT_NAME']}.s37")
    out_file_bin = os.path.join(os.getcwd(), f"{project_build_path}", f"{param_data['CONFIG_PROJECT_NAME']}_isp.bin")
    dst_dir = param_data["BIN_OUTPUT_DIR"]
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir, exist_ok=True)

    destination_path = os.path.join(dst_dir, param_data['CONFIG_PROJECT_NAME'] + "_QIO_" + param_data['CONFIG_PROJECT_VERSION'] + ".bin")
    shutil.copy(out_file_bin, destination_path)

    destination_path = os.path.join(dst_dir, param_data['CONFIG_PROJECT_NAME'] + "_" + param_data['CONFIG_PROJECT_VERSION'] + ".s37")
    shutil.copy(out_file_s37, destination_path)

    destination_path = os.path.join(dst_dir, param_data['CONFIG_PROJECT_NAME'] + "_" + param_data['CONFIG_PROJECT_VERSION'] + ".elf")
    shutil.copy(out_file_elf, destination_path)
    
    sys.exit(0)

if __name__ == "__main__":
    main()
