# macOS disk image: Recrayon-<version>-macOS.dmg (CPack DragNDrop).
#
#   cmake --build --preset <preset> --target package
#
# The install rules put Recrayon.app at the root of the prefix and run macdeployqt (see
# src/CMakeLists.txt). This module signs the bundle and configures the .dmg, which shows
# Recrayon.app next to a link to /Applications.
#
# Signing: macdeployqt rewrites the binaries, which invalidates their signatures, and Apple
# Silicon refuses to run unsigned code. By default the bundle is signed ad hoc ("-"): it runs on
# the machine that built it and, downloaded, after "Open" in the Finder context menu. For
# distribution pass a Developer ID identity:
#
#   -DRECRAYON_MACOS_SIGN_IDENTITY="Developer ID Application: Name (TEAMID)"
#
# which also enables the hardened runtime (with packaging/macos/Recrayon.entitlements, needed for
# the microphone). Notarization (xcrun notarytool, then stapler) is a separate step.

set(RECRAYON_MACOS_SIGN_IDENTITY "-" CACHE STRING
    "codesign identity for Recrayon.app (\"-\" = ad hoc)")

set(RECRAYON_MACOS_ENTITLEMENTS "${PROJECT_SOURCE_DIR}/packaging/macos/Recrayon.entitlements")
install(CODE "
    set(app \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/Recrayon.app\")
    set(identity \"${RECRAYON_MACOS_SIGN_IDENTITY}\")
    set(options --force --deep --sign \"\${identity}\")
    if(NOT identity STREQUAL \"-\")
        list(APPEND options --options runtime --timestamp
            --entitlements \"${RECRAYON_MACOS_ENTITLEMENTS}\")
    endif()
    message(STATUS \"Signing \${app} (identity: \${identity})\")
    execute_process(COMMAND codesign \${options} \"\${app}\" COMMAND_ERROR_IS_FATAL ANY)
")

set(CPACK_GENERATOR "DragNDrop")
set(CPACK_PACKAGE_NAME "Recrayon")
set(CPACK_PACKAGE_VENDOR "Recrayon")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME "Recrayon-${PROJECT_VERSION}-macOS")
set(CPACK_DMG_VOLUME_NAME "Recrayon")
set(CPACK_DMG_FORMAT "UDZO")

include(CPack)
