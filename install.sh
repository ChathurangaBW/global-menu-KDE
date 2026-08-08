#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

REPO_URL="${GLOBAL_MENU_KDE_REPO:-https://github.com/ChathurangaBW/global-menu-KDE.git}"
REF="${GLOBAL_MENU_KDE_REF:-main}"
INSTALL_DEPS=1
UNINSTALL=0
TEMP_DIR=""

usage() {
    cat <<'EOF'
Global Menu KDE portable Plasma 6 source installer

Usage:
  bash ./install.sh [options]
  curl -fsSL https://raw.githubusercontent.com/ChathurangaBW/global-menu-KDE/main/install.sh | bash

Options:
  --no-deps       Do not install build dependencies. Use this on unsupported
                  distributions after installing the dependencies manually.
  --ref REF       Git branch/tag/ref to fetch when the installer is piped or
                  run outside a repository checkout. Default: main.
  --uninstall     Remove a previous source-installer installation.
  -h, --help      Show this help.

Environment:
  GLOBAL_MENU_KDE_REPO   Override the Git repository URL.
  GLOBAL_MENU_KDE_REF    Override the default Git ref.

Run this script as your regular desktop user, not with sudo. It elevates only
for dependency installation and the final system-wide /usr installation.
EOF
}

fail() {
    printf 'global-menu-kde: %s\n' "$*" >&2
    exit 1
}

cleanup() {
    if [[ -n "${TEMP_DIR}" && -d "${TEMP_DIR}" ]]; then
        rm -rf "${TEMP_DIR}"
    fi
}
trap cleanup EXIT

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-deps)
            INSTALL_DEPS=0
            shift
            ;;
        --ref)
            [[ $# -ge 2 ]] || fail "--ref requires a value"
            REF="$2"
            shift 2
            ;;
        --uninstall)
            UNINSTALL=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            fail "unknown option: $1 (use --help)"
            ;;
    esac
done

if [[ ${EUID:-$(id -u)} -eq 0 ]]; then
    fail "run this installer as your regular desktop user, without sudo"
fi

if command -v sudo >/dev/null 2>&1; then
    ROOT=(sudo)
elif command -v doas >/dev/null 2>&1; then
    ROOT=(doas)
else
    fail "sudo or doas is required for system-wide installation"
fi

as_root() {
    "${ROOT[@]}" "$@"
}

if [[ ${UNINSTALL} -eq 1 ]]; then
    if command -v global-menu-kde-uninstall >/dev/null 2>&1; then
        exec global-menu-kde-uninstall
    fi

    script_path="${BASH_SOURCE[0]:-}"
    if [[ -n "${script_path}" && -f "${script_path}" ]]; then
        script_dir="$(cd "$(dirname "${script_path}")" && pwd)"
        if [[ -f "${script_dir}/scripts/uninstall-system.sh" ]]; then
            exec bash "${script_dir}/scripts/uninstall-system.sh"
        fi
    fi

    fail "installed uninstaller not found; reinstall once, then run --uninstall"
fi

PLASMASHELL_PATH="$(command -v plasmashell || true)"
if [[ -z "${PLASMASHELL_PATH}" ]]; then
    fail "KDE Plasma 6 is required (plasmashell was not found)"
fi
printf 'Detected Plasma runtime: %s\n' "${PLASMASHELL_PATH}"
printf 'CMake will verify Plasma 6 development compatibility during configure.\n'

install_dependencies() {
    if command -v apt-get >/dev/null 2>&1; then
        printf 'Installing build dependencies with apt...\n'
        as_root apt-get update
        as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
            git build-essential cmake ninja-build extra-cmake-modules \
            qt6-base-dev qt6-base-dev-tools qt6-declarative-dev \
            libkf6config-dev libkf6coreaddons-dev libkf6i18n-dev libkf6windowsystem-dev \
            libplasma-dev plasma-workspace-dev plasma-workspace dbus
        return
    fi

    if command -v dnf >/dev/null 2>&1; then
        printf 'Installing build dependencies with dnf...\n'
        as_root dnf -y install \
            git gcc-c++ cmake ninja-build extra-cmake-modules \
            qt6-qtbase-devel qt6-qtdeclarative-devel \
            kf6-kconfig-devel kf6-kcoreaddons-devel kf6-ki18n-devel kf6-kwindowsystem-devel \
            libplasma-devel plasma-workspace-devel plasma-workspace dbus-daemon
        return
    fi

    if command -v pacman >/dev/null 2>&1; then
        printf 'Installing build dependencies with pacman...\n'
        as_root pacman -Syu --needed --noconfirm \
            git base-devel cmake ninja extra-cmake-modules \
            qt6-base qt6-declarative \
            kconfig ki18n kwindowsystem libplasma plasma-workspace dbus
        return
    fi

    if command -v zypper >/dev/null 2>&1; then
        printf 'Installing build dependencies with zypper...\n'
        as_root zypper --non-interactive install --no-recommends \
            git gcc-c++ cmake ninja kf6-extra-cmake-modules \
            qt6-base-devel qt6-declarative-devel \
            kf6-kconfig-devel kf6-kcoreaddons-devel kf6-ki18n-devel kf6-kwindowsystem-devel \
            libplasma6-devel plasma6-workspace-devel plasma6-workspace dbus-1 dbus-1-tools
        return
    fi

    fail "unsupported package manager; install the build dependencies manually and rerun with --no-deps"
}

if [[ ${INSTALL_DEPS} -eq 1 ]]; then
    install_dependencies
fi

for command_name in git cmake ninja c++; do
    command -v "${command_name}" >/dev/null 2>&1 || \
        fail "required build tool is missing: ${command_name}"
done

SOURCE_DIR=""
script_path="${BASH_SOURCE[0]:-}"
if [[ -n "${script_path}" && -f "${script_path}" ]]; then
    script_dir="$(cd "$(dirname "${script_path}")" && pwd)"
    if [[ -f "${script_dir}/CMakeLists.txt" && -f "${script_dir}/scripts/install-system.sh" ]]; then
        SOURCE_DIR="${script_dir}"
        printf 'Using local source tree: %s\n' "${SOURCE_DIR}"
    fi
fi

if [[ -z "${SOURCE_DIR}" ]]; then
    TEMP_DIR="$(mktemp -d -t global-menu-kde.XXXXXXXX)"
    SOURCE_DIR="${TEMP_DIR}/source"
    printf 'Fetching %s (%s)...\n' "${REPO_URL}" "${REF}"
    git init -q "${SOURCE_DIR}"
    git -C "${SOURCE_DIR}" remote add origin "${REPO_URL}"
    git -C "${SOURCE_DIR}" fetch --depth 1 origin "${REF}"
    git -C "${SOURCE_DIR}" checkout -q --detach FETCH_HEAD
fi

bash "${SOURCE_DIR}/scripts/install-system.sh"

cat <<'EOF'

Global Menu KDE installed successfully.
Log out of Plasma and back in once, then:
  Edit Mode -> Add Widgets -> Global Menu KDE

Uninstall later with:
  global-menu-kde-uninstall
EOF
