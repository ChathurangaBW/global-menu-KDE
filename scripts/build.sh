#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ChathurangaBW
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build"}

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF
cmake --build "$BUILD_DIR" --parallel