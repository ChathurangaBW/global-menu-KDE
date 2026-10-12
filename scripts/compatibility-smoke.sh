#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Run only inside the disposable compatibility CI container, never on a desktop.
set -euo pipefail

[[ ${GITHUB_ACTIONS:-} == true && ${EUID} -eq 0 ]] || {
    echo 'This helper requires a root GitHub Actions job container.' >&2
    exit 1
}
distro=${1:?distribution identifier required}
family=${2:?package family required}
[[ $distro =~ ^[a-z0-9-]+$ ]] || exit 1
case "$family" in deb|rpm|suse|arch) ;; *) exit 1 ;; esac
root=$PWD
logs="$root/compatibility-logs"
dist="$root/compatibility-dist"
build="$root/build-compatibility-$distro"
stage="$root/stage-compatibility-$distro"
mkdir -p "$logs" "$dist" "$build/version-probe"
installed=0
xvfb_pid=''

uninstall() {
    case "$family" in
        deb) dpkg -r global-menu-kde ;;
        rpm|suse) rpm -e global-menu-kde ;;
        arch)
            while IFS= read -r path; do
                [[ $path == /usr/* ]] || return 1
                rm -f -- "$path" || return 1
            done < "$build/install_manifest.txt"
            ;;
    esac
}

finish() {
    local result=$? cleanup_result=0
    trap - EXIT
    set +e
    if [[ -n $xvfb_pid ]]; then
        kill "$xvfb_pid" 2>/dev/null
        wait "$xvfb_pid" 2>/dev/null
    fi
    if [[ $installed == 1 ]]; then
        uninstall > "$logs/$distro-uninstall.log" 2>&1
        cleanup_result=$?
        cat "$logs/$distro-uninstall.log"
        if [[ $cleanup_result == 0 ]]; then
            while IFS= read -r path; do
                if [[ -e $path || -L $path ]]; then
                    echo "Installed file remains after uninstall: $path"
                    cleanup_result=1
                fi
            done < "$build/install_manifest.txt"
        fi
    fi
    for file in CMakeCache.txt CMakeFiles/CMakeConfigureLog.yaml Testing/Temporary/LastTest.log install_manifest.txt; do
        if [[ -f $build/$file ]]; then
            cp "$build/$file" "$logs/$distro-${file##*/}"
        fi
    done
    if [[ $result == 0 && $cleanup_result != 0 ]]; then result=$cleanup_result; fi
    exit "$result"
}
trap finish EXIT

# The project currently has lower KF/Plasma minima. Enforce this matrix's
# supported development baseline independently without modifying its CMake.
cat > "$build/version-probe/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.22)
project(compatibility_versions LANGUAGES CXX)
find_package(ECM 6.5 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
find_package(Qt6 6.6 REQUIRED COMPONENTS Core DBus Gui Quick Widgets Test)
find_package(KF6 6.5 REQUIRED COMPONENTS Config CoreAddons I18n WindowSystem)
find_package(Plasma 6.5 REQUIRED)
find_package(LibTaskManager REQUIRED CONFIG)
message(STATUS "Compatibility: Qt=${Qt6_VERSION}, KF=${KF6_VERSION}, Plasma=${Plasma_VERSION}, ECM=${ECM_VERSION}")
CMAKE
cmake --version
c++ --version
msgfmt --version
cmake -S "$build/version-probe" -B "$build/version-probe/build" -G Ninja \
    2>&1 | tee "$logs/$distro-development-versions.log"
cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=ON \
    2>&1 | tee "$logs/$distro-configure.log"
cmake --build "$build" --parallel 2 2>&1 | tee "$logs/$distro-build.log"
ctest --test-dir "$build" --output-on-failure 2>&1 | tee "$logs/$distro-ctest.log"

# Staging both verifies the install manifest and supports the Arch source smoke.
DESTDIR="$stage" cmake --install "$build"
case "$family" in
    deb|rpm|suse)
        generator=RPM
        extension=rpm
        if [[ $family == deb ]]; then generator=DEB; extension=deb; fi
        cpack --config "$build/CPackConfig.cmake" -G "$generator" -B "$build/packages" \
            2>&1 | tee "$logs/$distro-cpack.log"
        mapfile -t packages < <(find "$build/packages" -maxdepth 1 -type f -name "*.$extension")
        [[ ${#packages[@]} == 1 ]]
        package="$dist/$distro-$(basename "${packages[0]}")"
        cp "${packages[0]}" "$package"
        # Mark before installation so partial installation is cleaned up too.
        installed=1
        case "$family" in
            deb)
                dpkg-deb --info "$package"
                dpkg-deb --contents "$package"
                apt-get install -y "$package"
                dpkg-query -W global-menu-kde
                ;;
            rpm)
                rpm -qpi "$package"
                rpm -qpl "$package"
                dnf -y install "$package"
                rpm -q global-menu-kde
                ;;
            suse)
                rpm -qpi "$package"
                rpm -qpl "$package"
                # Only the locally generated unsigned RPM bypasses signature checks.
                zypper --non-interactive --no-gpg-checks install "$package"
                rpm -q global-menu-kde
                ;;
        esac
        ;;
    arch)
        tar -C "$stage" -czf "$dist/$distro-$(uname -m)-cmake-stage.tar.gz" .
        installed=1
        cmake --install "$build"
        ;;
esac

mapfile -t plugins < <(find /usr -type f -name org.chathuranga.globalmenu.so)
[[ ${#plugins[@]} == 1 ]]
file "${plugins[0]}"
ldd "${plugins[0]}" 2>&1 | tee "$logs/$distro-ldd.log"
[[ -s $logs/$distro-ldd.log ]]
if grep -Fq 'not found' "$logs/$distro-ldd.log"; then exit 1; fi
mo=/usr/share/locale/fr/LC_MESSAGES/plasma_applet_org.chathuranga.globalmenu.mo
[[ -s $mo ]]
msgunfmt "$mo" > "$logs/$distro-installed-fr.po"
grep -q '^msgstr "[^"].*"' "$logs/$distro-installed-fr.po"

# Prefer an X11-backed applet smoke where Xvfb is available. Rocky Linux 10
# does not currently ship Xvfb in its enabled BaseOS/AppStream/CRB/EPEL set,
# so retain a real plasmawindowed loader check through Qt's offscreen backend.
unset QT_PLUGIN_PATH QML2_IMPORT_PATH QML_IMPORT_PATH
export LIBGL_ALWAYS_SOFTWARE=1
export XDG_RUNTIME_DIR="$build/runtime"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
if command -v Xvfb >/dev/null 2>&1; then
    export QT_QPA_PLATFORM=xcb
    Xvfb -displayfd 3 -screen 0 1280x800x24 -nolisten tcp \
        3> "$build/x-display" > "$logs/$distro-xvfb.log" 2>&1 &
    xvfb_pid=$!
    for ((attempt=0; attempt<100; attempt++)); do
        [[ -s $build/x-display ]] && break
        kill -0 "$xvfb_pid"
        sleep 0.1
    done
    [[ -s $build/x-display ]]
    DISPLAY=":$(cat "$build/x-display")"
    export DISPLAY
else
    export QT_QPA_PLATFORM=offscreen
    unset DISPLAY
    printf '%s\n' 'Xvfb unavailable; using Qt offscreen platform.' > "$logs/$distro-xvfb.log"
fi
status=0
timeout --signal=TERM --kill-after=5s 20s dbus-run-session -- \
    plasmawindowed org.chathuranga.globalmenu > "$logs/$distro-plasmawindowed.log" 2>&1 || status=$?
cat "$logs/$distro-plasmawindowed.log"
# A GUI must stay alive until the timeout, not exit early with a misleading zero.
[[ $status == 124 ]]
if grep -Eiq 'failed to load|cannot load|could not load|plugin.*not found|module .* is not installed|QQmlApplicationEngine failed|Type .* unavailable|ReferenceError|TypeError|SyntaxError|Error loading|error while loading shared libraries|segmentation fault|assertion.*failed' \
    "$logs/$distro-plasmawindowed.log"; then
    echo 'Installed Plasma applet smoke rejected runtime errors.' >&2
    exit 1
fi
# EXIT always uninstalls and checks the complete install manifest, preserving any
# earlier failure and making an uninstall failure fail an otherwise green job.
