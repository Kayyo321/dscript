#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_DIR="${BUILD_DIR:-build}"

show_help() {
    cat <<EOF
Usage: ./build_and_run.sh [--build-type TYPE] [--build-dir DIR] [--] [program args...]

Examples:
  ./build_and_run.sh
  ./build_and_run.sh examples/01_literals_and_ops.dsr
  ./build_and_run.sh --build-type Release --build-dir build-release examples/23_imports_and_modules.dsr
EOF
}

PROGRAM_ARGS=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-type)
            BUILD_TYPE="${2:-}"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="${2:-}"
            shift 2
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        --)
            shift
            PROGRAM_ARGS=("$@")
            break
            ;;
        *)
            PROGRAM_ARGS+=("$1")
            shift
            ;;
    esac
done

"${SCRIPT_DIR}/build.sh" "${BUILD_TYPE}" "${BUILD_DIR}"

EXECUTABLE="${SCRIPT_DIR}/${BUILD_DIR}/dscript"
if [[ ! -x "${EXECUTABLE}" ]]; then
    echo "Error: executable not found: ${EXECUTABLE}"
    exit 1
fi

echo "Running: ${EXECUTABLE} ${PROGRAM_ARGS[*]:-}"
"${EXECUTABLE}" "${PROGRAM_ARGS[@]}"
