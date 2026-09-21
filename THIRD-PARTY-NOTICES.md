# Third-party notices

Recrayon is licensed under CC BY-NC-ND 4.0 (see `LICENSE`). The installers and packages also
contain the third-party software below, which keeps its own license. Those licenses are not
changed by Recrayon's license: in particular, you may replace or modify these libraries in your
copy of Recrayon, and reverse engineer it to debug such modifications, as the LGPL allows.

The full license texts are included next to this file (`LGPL-3.0.txt`, `GPL-3.0.txt`).

## Qt 6.8.3

- Modules used: Qt Core, Gui, Widgets, Network, Svg, Multimedia and, on Linux, DBus, XcbQpa,
  OpenGL and their plugins (platform, image formats, multimedia, TLS...).
- License: GNU Lesser General Public License v3.0 (LGPL-3.0), which incorporates the GNU General
  Public License v3.0 (GPL-3.0).
- Copyright (C) The Qt Company Ltd. and other contributors.
- Linked dynamically: the Qt libraries are separate files (`Qt6*.dll`, `libQt6*.so*`,
  `Qt*.framework`) that can be replaced with a compatible build.
- Source code: https://download.qt.io/official_releases/qt/6.8/6.8.3/submodules/
- Qt itself contains third-party code (FreeType, HarfBuzz, libpng, libjpeg, zlib, PCRE2, and
  others) under their own permissive licenses, listed at
  https://doc.qt.io/qt-6.8/licenses-used-in-qt.html

## FFmpeg 7.1

- Used by Qt Multimedia to encode the screen recordings.
- License: GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later), built without
  GPL or non-free components by the Qt Company.
- Copyright (C) the FFmpeg developers.
- Linked dynamically (`avcodec`, `avformat`, `avutil`, `swresample`, `swscale`).
- Source code: https://ffmpeg.org/download.html (release 7.1).

## ICU 73 (Linux packages)

- Unicode support used by Qt Core.
- License: Unicode License v3 (https://www.unicode.org/license.txt).
- Copyright (C) Unicode, Inc. and others.

## MinGW-w64 runtime (Windows)

- `libstdc++-6.dll`, `libgcc_s_seh-1.dll`: GNU GPL v3 with the GCC Runtime Library Exception,
  which allows distributing them with programs under any license.
- `libwinpthread-1.dll`: MIT-style license of the MinGW-w64 project.
- Source code: https://www.mingw-w64.org/ and https://gcc.gnu.org/
