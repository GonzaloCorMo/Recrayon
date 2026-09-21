# Windows installer (Inno Setup).
#
#   cmake --build --preset <preset> --target installer
#
# 1. `cmake --install` into <build>/installer/staging: recrayon.exe + the Qt runtime deployed by
#    the install rules (see src/CMakeLists.txt).
# 2. ISCC compiles packaging/windows/Recrayon.iss (configured here) into
#    <build>/installer/Recrayon-<version>-setup.exe.
#
# Use a Release build for installers that are handed out (preset "release").

set(RECRAYON_ICON_FILE "${PROJECT_SOURCE_DIR}/packaging/windows/recrayon.ico")
set(RECRAYON_INSTALLER_DIR "${CMAKE_BINARY_DIR}/installer")
set(RECRAYON_STAGING_DIR "${RECRAYON_INSTALLER_DIR}/staging")
set(RECRAYON_INSTALLER_OUTPUT_DIR "${RECRAYON_INSTALLER_DIR}")

set(program_files_x86 "ProgramFiles(x86)")
find_program(RECRAYON_ISCC ISCC
    HINTS
        "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6"
        "$ENV{${program_files_x86}}/Inno Setup 6"
        "$ENV{ProgramFiles}/Inno Setup 6"
    DOC "Inno Setup command-line compiler (ISCC.exe)")

configure_file(
    "${PROJECT_SOURCE_DIR}/packaging/windows/Recrayon.iss.in"
    "${RECRAYON_INSTALLER_DIR}/Recrayon.iss"
    @ONLY)

if(RECRAYON_ISCC)
    add_custom_target(installer
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${RECRAYON_STAGING_DIR}"
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
                --prefix "${RECRAYON_STAGING_DIR}" --config "$<CONFIG>"
        COMMAND "${RECRAYON_ISCC}" /Q "${RECRAYON_INSTALLER_DIR}/Recrayon.iss"
        DEPENDS recrayon
        WORKING_DIRECTORY "${RECRAYON_INSTALLER_DIR}"
        COMMENT "Building ${RECRAYON_INSTALLER_DIR}/Recrayon-${PROJECT_VERSION}-setup.exe"
        VERBATIM)
else()
    message(STATUS "Inno Setup (ISCC.exe) not found: the 'installer' target is not available")
endif()
