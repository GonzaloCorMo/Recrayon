# Linux build environment for Recrayon packages (.deb and AppImage).
#
# Ubuntu 22.04 (glibc 2.35) so the packages also run on older distributions, Qt from the
# official binaries (aqtinstall), linuxdeploy for the AppImage. Used by
# packaging/linux/build-packages.sh.
#
#   docker build -t recrayon-linux-builder packaging/linux
#   docker run --rm -v "$PWD:/src" recrayon-linux-builder bash packaging/linux/build-packages.sh

FROM ubuntu:22.04

ARG QT_VERSION=6.8.3
ENV DEBIAN_FRONTEND=noninteractive

# Compiler, packaging tools (dpkg-shlibdeps for the .deb) and the X11/GL/audio libraries the Qt
# xcb and multimedia plugins load at runtime (also needed to run the tests and the app).
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential ca-certificates dpkg-dev file git python3-pip wget \
        libasound2 libdbus-1-3 libegl1 libfontconfig1 libgl1-mesa-dev libglib2.0-0 libpulse0 \
        libx11-xcb1 libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 \
        libxcb-render-util0 libxcb-shape0 libxcb-shm0 libxcb-sync1 libxcb-xfixes0 \
        libxcb-xinerama0 libxcb-xkb1 libxkbcommon-dev libxkbcommon-x11-0 libxrandr2 \
        xauth xvfb \
    && rm -rf /var/lib/apt/lists/*

# CMake >= 3.25 (22.04 ships 3.22) and Ninja from PyPI; Qt with Multimedia (screen recording).
RUN pip3 install --no-cache-dir aqtinstall cmake ninja \
    && aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 -m qtmultimedia -O /opt/Qt \
    && rm -f aqtinstall.log

# linuxdeploy + Qt plugin. No FUSE inside containers: the AppImages extract themselves.
RUN mkdir -p /opt/linuxdeploy \
    && cd /opt/linuxdeploy \
    && wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
    && wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage \
    && chmod +x *.AppImage

ENV APPIMAGE_EXTRACT_AND_RUN=1 \
    PATH="/opt/linuxdeploy:${PATH}" \
    CMAKE_PREFIX_PATH="/opt/Qt/${QT_VERSION}/gcc_64"

WORKDIR /src
