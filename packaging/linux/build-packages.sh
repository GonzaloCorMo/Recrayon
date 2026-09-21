#!/usr/bin/env bash
# Builds, tests and packages Recrayon for Linux: build/linux-release/recrayon_<v>_amd64.deb and
# build/linux-release/appimage/Recrayon-<v>-x86_64.AppImage.
#
# Meant to run inside packaging/linux/Dockerfile (Qt in CMAKE_PREFIX_PATH, linuxdeploy in PATH),
# from the repository root:
#
#   docker build -t recrayon-linux-builder packaging/linux
#   docker run --rm -v "$PWD:/src" recrayon-linux-builder bash packaging/linux/build-packages.sh
#
# Also works on a Linux machine with the same tools installed.
set -euo pipefail

build_dir="${BUILD_DIR:-build/linux-release}"

cmake -S . -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DRECRAYON_BUILD_TESTS=ON
cmake --build "$build_dir"
ctest --test-dir "$build_dir" --output-on-failure

cmake --build "$build_dir" --target package
cmake --build "$build_dir" --target appimage

echo
echo "Packages:"
ls -lh "$build_dir"/*.deb "$build_dir"/appimage/*.AppImage
