#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
version=$(sed -n 's/^project(global-menu-kde VERSION \([^ ]*\).*/\1/p' "$repo_root/CMakeLists.txt")
if [[ -z "$version" ]]; then
    echo "Could not determine project version from CMakeLists.txt" >&2
    exit 1
fi

assets_dir=${RELEASE_ASSETS_DIR:-"$repo_root/release-assets"}
archive_name="global-menu-kde-${version}.tar.gz"
prefix="global-menu-kde-${version}/"

rm -rf -- "$assets_dir"
mkdir -p -- "$assets_dir"

git -C "$repo_root" archive --format=tar.gz --prefix="$prefix" -o "$assets_dir/$archive_name" HEAD

(
    cd "$assets_dir"
    sha256sum "$archive_name" > SHA256SUMS
    sha256sum -c SHA256SUMS
)

printf 'Created %s and SHA256SUMS in %s\n' "$archive_name" "$assets_dir"
