#!/usr/bin/env bash
set -euo pipefail

BUILD_ROOT="${1:-build-release}"
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

declare -a SUCCESS_TARGETS=()
declare -a SKIPPED_TARGETS=()
declare -a FAILED_TARGETS=()

command_exists() {
	command -v "$1" >/dev/null 2>&1
}

run_target_build() {
	local label="$1"
	local build_dir="$2"
	local system_name="$3"
	local processor="$4"
	local c_compiler="$5"
	local cxx_compiler="$6"

	local -a configure_args=(
		-S "${SCRIPT_DIR}"
		-B "${build_dir}"
		-DCMAKE_BUILD_TYPE=Release
	)

	if [[ -n "${system_name}" ]]; then
		configure_args+=("-DCMAKE_SYSTEM_NAME=${system_name}")
	fi
	if [[ -n "${processor}" ]]; then
		configure_args+=("-DCMAKE_SYSTEM_PROCESSOR=${processor}")
	fi

	if [[ -n "${c_compiler}" || -n "${cxx_compiler}" ]]; then
		if [[ -z "${c_compiler}" || -z "${cxx_compiler}" ]]; then
			echo "Skipping ${label}: both C and C++ compilers are required"
			SKIPPED_TARGETS+=("${label}")
			return 0
		fi
		if ! command_exists "${c_compiler}" || ! command_exists "${cxx_compiler}"; then
			echo "Skipping ${label}: missing compiler(s) ${c_compiler}/${cxx_compiler}"
			SKIPPED_TARGETS+=("${label}")
			return 0
		fi
		configure_args+=("-DCMAKE_C_COMPILER=${c_compiler}")
		configure_args+=("-DCMAKE_CXX_COMPILER=${cxx_compiler}")
	fi

	echo ""
	echo "=== ${label} ==="
	echo "Configuring: ${build_dir}"

	if ! cmake "${configure_args[@]}"; then
		echo "Failed configure: ${label}"
		FAILED_TARGETS+=("${label}")
		return 0
	fi

	echo "Building all targets (Release): ${label}"
	if ! cmake --build "${build_dir}" --target all --parallel; then
		echo "Failed build: ${label}"
		FAILED_TARGETS+=("${label}")
		return 0
	fi

	SUCCESS_TARGETS+=("${label}")
	echo "Completed: ${label}"
}

# linux targets
run_target_build "linux-x86_64 (amd64/intel64)" "${SCRIPT_DIR}/${BUILD_ROOT}/linux-x86_64" "Linux" "x86_64" "" ""
run_target_build "linux-x86 (intel32)" "${SCRIPT_DIR}/${BUILD_ROOT}/linux-x86" "Linux" "x86" "i686-linux-gnu-gcc" "i686-linux-gnu-g++"
run_target_build "linux-arm64" "${SCRIPT_DIR}/${BUILD_ROOT}/linux-arm64" "Linux" "aarch64" "aarch64-linux-gnu-gcc" "aarch64-linux-gnu-g++"

# windows targets
run_target_build "windows-x86_64 (amd64/intel64)" "${SCRIPT_DIR}/${BUILD_ROOT}/windows-x86_64" "Windows" "x86_64" "x86_64-w64-mingw32-gcc" "x86_64-w64-mingw32-g++"
run_target_build "windows-x86 (intel32)" "${SCRIPT_DIR}/${BUILD_ROOT}/windows-x86" "Windows" "x86" "i686-w64-mingw32-gcc" "i686-w64-mingw32-g++"
run_target_build "windows-arm64" "${SCRIPT_DIR}/${BUILD_ROOT}/windows-arm64" "Windows" "ARM64" "aarch64-w64-mingw32-gcc" "aarch64-w64-mingw32-g++"

# macos targets (requires osxcross-style toolchains)
run_target_build "macos-x86_64 (intel64)" "${SCRIPT_DIR}/${BUILD_ROOT}/macos-x86_64" "Darwin" "x86_64" "o64-clang" "o64-clang++"
run_target_build "macos-arm64" "${SCRIPT_DIR}/${BUILD_ROOT}/macos-arm64" "Darwin" "arm64" "oa64-clang" "oa64-clang++"

echo ""
echo "========== Build Summary =========="
if [[ ${#SUCCESS_TARGETS[@]} -gt 0 ]]; then
	echo "Succeeded (${#SUCCESS_TARGETS[@]}):"
	printf '  - %s\n' "${SUCCESS_TARGETS[@]}"
else
	echo "Succeeded (0)"
fi

if [[ ${#SKIPPED_TARGETS[@]} -gt 0 ]]; then
	echo "Skipped (${#SKIPPED_TARGETS[@]}):"
	printf '  - %s\n' "${SKIPPED_TARGETS[@]}"
else
	echo "Skipped (0)"
fi

if [[ ${#FAILED_TARGETS[@]} -gt 0 ]]; then
	echo "Failed (${#FAILED_TARGETS[@]}):"
	printf '  - %s\n' "${FAILED_TARGETS[@]}"
	exit 1
else
	echo "Failed (0)"
fi

echo "Release matrix build complete under: ${BUILD_ROOT}"
