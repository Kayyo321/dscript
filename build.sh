#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${1:-Debug}"
BUILD_DIR="${2:-build}"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

if [[ "$(uname -s)" == "Darwin" ]]; then
	APPLE_CC="$(xcrun --find clang)"
	APPLE_CXX="$(xcrun --find clang++)"
	APPLE_SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"

	CC="${CC:-${APPLE_CC}}"
	CXX="${CXX:-${APPLE_CXX}}"
	SDKROOT="${SDKROOT:-${APPLE_SDKROOT}}"

	export CC CXX SDKROOT
fi

echo "Configuring: type=${BUILD_TYPE}, dir=${BUILD_DIR}"
cmake -S "${SCRIPT_DIR}" -B "${SCRIPT_DIR}/${BUILD_DIR}" \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
	-DCMAKE_C_COMPILER="${CC:-}" \
	-DCMAKE_CXX_COMPILER="${CXX:-}" \
	-DCMAKE_OSX_SYSROOT="${SDKROOT:-}"

echo "Building..."
cmake --build "${SCRIPT_DIR}/${BUILD_DIR}" --parallel

echo "Build complete: ${BUILD_DIR}"