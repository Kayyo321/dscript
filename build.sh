#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${1:-Debug}"
BUILD_DIR="${2:-build}"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

echo "Configuring: type=${BUILD_TYPE}, dir=${BUILD_DIR}"
cmake -S "${SCRIPT_DIR}" -B "${SCRIPT_DIR}/${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "Building..."
cmake --build "${SCRIPT_DIR}/${BUILD_DIR}" --parallel

echo "Build complete: ${BUILD_DIR}"