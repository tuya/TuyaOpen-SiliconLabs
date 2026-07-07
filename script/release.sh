#!/usr/bin/env bash

set -euo pipefail

if [[ -n ${BASH_SOURCE[0]:-} ]]; then
    script_path="${BASH_SOURCE[0]}"
else
    script_path="$0"
fi
script_dir="$(realpath "$(dirname "${script_path}")")"
repo_dir="$(dirname "${script_dir}")"

usage() {
    echo "Usage: $(basename "${script_path}") <destination-directory>"
    echo
    echo "Copy platform source to <destination-directory> without git history."
    echo "Excluded: sdks/, tools/, __pycache__/, .github/, Dockerfile, .gitmodules"
    exit 1
}

if [[ $# -ne 1 ]]; then
    usage
fi

dst="$(realpath -m "$1")"

if [[ "${dst}" == "${repo_dir}" ]] || [[ "${dst}" == "${repo_dir}/"* ]]; then
    echo "Error: destination must be outside the source repository: ${repo_dir}" >&2
    exit 1
fi

mkdir -p "${dst}"

echo "Source:      ${repo_dir}"
echo "Destination: ${dst}"
echo

rsync -a \
    --exclude='.git/' \
    --exclude='sdks/' \
    --exclude='tools/' \
    --exclude='__pycache__/' \
    --exclude='.github/' \
    --exclude='Dockerfile' \
    --exclude='.gitmodules' \
    "${repo_dir}/" "${dst}/"

echo
echo "Release copy completed."
