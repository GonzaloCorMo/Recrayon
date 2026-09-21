# Linux packages: .deb (CPack) and AppImage (linuxdeploy).
#
#   cmake --build --preset <preset> --target package    # recrayon_<version>_amd64.deb
#   cmake --build --preset <preset> --target appimage   # Recrayon-<version>-x86_64.AppImage
#
# Both bundle Qt: distributions rarely ship Qt >= 6.8 with Multimedia. The reproducible way to
# build them is packaging/linux/build-packages.sh inside packaging/linux/Dockerfile
# (Ubuntu 22.04, so they run on glibc >= 2.35).
#
# Included from src/CMakeLists.txt after the install rules of the executable and Qt runtime.

# ---- Desktop integration -----------------------------------------------------
# Component "desktop" so the AppImage can install only these (linuxdeploy deploys Qt itself).
install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/recrayon.desktop"
    DESTINATION share/applications
    COMPONENT desktop)
install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/recrayon.png"
    DESTINATION share/icons/hicolor/256x256/apps
    COMPONENT desktop)

# <prefix>/bin/recrayon -> ../lib/recrayon/recrayon. The executable finds its Qt through
# $ORIGIN, which the loader resolves to the real location of the file.
install(CODE [[
    set(bin_dir "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin")
    file(MAKE_DIRECTORY "${bin_dir}")
    file(CREATE_LINK "../lib/recrayon/recrayon" "${bin_dir}/recrayon" SYMBOLIC)
    message(STATUS "Installing: ${bin_dir}/recrayon -> ../lib/recrayon/recrayon")
]])

# ---- .deb (CPack) ------------------------------------------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "recrayon")
set(CPACK_PACKAGE_VENDOR "Recrayon")
set(CPACK_PACKAGE_CONTACT "Recrayon")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
    "Draw annotations on top of your screen while you keep using it: a transparent overlay per\n\
monitor with a drawing mode and a click-through mode, pen, shapes, text, whiteboard, replay,\n\
screenshots and screen recording. Bundles its own copy of Qt.")
# System libraries (X11/xcb, GL, fontconfig, audio...) become dependencies via dpkg-shlibdeps;
# the bundled Qt libraries are found in the package itself.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "usr/lib/recrayon/lib")

include(CPack)

# ---- AppImage (linuxdeploy + linuxdeploy-plugin-qt) --------------------------
find_program(RECRAYON_LINUXDEPLOY
    NAMES linuxdeploy linuxdeploy-x86_64.AppImage
    DOC "linuxdeploy (https://github.com/linuxdeploy/linuxdeploy), with linuxdeploy-plugin-qt next to it")

if(RECRAYON_LINUXDEPLOY)
    set(appimage_dir "${CMAKE_BINARY_DIR}/appimage")
    set(appdir "${appimage_dir}/AppDir")
    set(appimage_file "Recrayon-${PROJECT_VERSION}-x86_64.AppImage")
    add_custom_target(appimage
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${appdir}"
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
                --prefix "${appdir}/usr" --component desktop
        COMMAND "${CMAKE_COMMAND}" -E env
                "QMAKE=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}/qmake"
                "LINUXDEPLOY_OUTPUT_VERSION=${PROJECT_VERSION}"
                "OUTPUT=${appimage_file}"
            "${RECRAYON_LINUXDEPLOY}"
                --appdir "${appdir}"
                --executable "$<TARGET_FILE:recrayon>"
                --desktop-file "${appdir}/usr/share/applications/recrayon.desktop"
                --icon-file "${PROJECT_SOURCE_DIR}/packaging/linux/recrayon.png"
                --plugin qt
                --output appimage
        DEPENDS recrayon
        WORKING_DIRECTORY "${appimage_dir}"
        COMMENT "Building ${appimage_dir}/${appimage_file}"
        VERBATIM)
    file(MAKE_DIRECTORY "${appimage_dir}")
else()
    message(STATUS "linuxdeploy not found: the 'appimage' target is not available")
endif()
