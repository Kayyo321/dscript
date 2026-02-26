#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

declare -a TARGETS=()
if [[ $# -gt 0 ]]; then
    TARGETS=("$@")
else
    TARGETS=("build" "build-release")
fi

for target in "${TARGETS[@]}"; do
    if [[ "${target}" = /* ]]; then
        resolved="${target}"
    else
        resolved="${SCRIPT_DIR}/${target}"
    fi

    if [[ -d "${resolved}" ]]; then
        echo "Removing ${resolved}"
        rm -rf "${resolved}"
    else
        echo "Skipping ${resolved} (not found)"
    fi
done

echo "Clean complete."
