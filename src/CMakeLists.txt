configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/Version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/recrayon/Version.h"
    @ONLY)

# ---- recrayon_core -----------------------------------------------------------
# Document model and drawing tools. Depends on Qt Core/Gui only (never Widgets),
# so it can be unit-tested headless.
add_library(recrayon_core STATIC
    config/SessionState.cpp
    config/SessionState.h
    config/Settings.cpp
    config/Settings.h
    core/CalloutItem.cpp
    core/CalloutItem.h
    core/Commands.cpp
    core/Commands.h
    core/DesktopLayout.cpp
    core/DesktopLayout.h
    core/Document.cpp
    core/Document.h
    core/FreehandItem.cpp
    core/FreehandItem.h
    core/Geometry.h
    core/Item.cpp
    core/Item.h
    core/ReplayTimeline.cpp
    core/ReplayTimeline.h
    core/ShapeItem.cpp
    core/ShapeItem.h
    core/StrokeStyle.h
    core/TextItem.cpp
    core/TextItem.h
    tools/CalloutTool.cpp
    tools/CalloutTool.h
    tools/EraserTool.cpp
    tools/EraserTool.h
    tools/MoveTool.cpp
    tools/MoveTool.h
    tools/PenTool.cpp
    tools/PenTool.h
    tools/ShapeTool.cpp
    tools/ShapeTool.h
    tools/TextTool.cpp
    tools/TextTool.h
    tools/Tool.h
    tools/ToolController.cpp
    tools/ToolController.h
    tools/ToolKind.h)

target_include_directories(recrayon_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(recrayon_core PUBLIC Qt6::Core Qt6::Gui)
recrayon_set_warnings(recrayon_core)

# ---- recrayon (application) --------------------------------------------------
qt_add_executable(recrayon
    main.cpp
    app/Application.cpp
    app/Application.h
    app/CaptureController.cpp
    app/CaptureController.h
    app/Translations.cpp
    app/Translations.h
    capture/Screenshot.cpp
    capture/Screenshot.h
    platform/CaptureExclusion.cpp
    platform/CaptureExclusion.h
    platform/CursorSprite.cpp
    platform/CursorSprite.h
    platform/DesktopGeometry.cpp
    platform/DesktopGeometry.h
    platform/GlobalHotkeys.cpp
    platform/GlobalHotkeys.h
    platform/TopLevelWindows.cpp
    platform/TopLevelWindows.h
    ui/AppActions.h
    ui/Icons.cpp
    ui/Icons.h
    ui/OverlayHint.cpp
    ui/OverlayHint.h
    ui/OverlayManager.cpp
    ui/OverlayManager.h
    ui/OverlayWindow.cpp
    ui/OverlayWindow.h
    ui/PointerHighlight.cpp
    ui/PointerHighlight.h
    ui/RecordingIndicator.cpp
    ui/RecordingIndicator.h
    ui/RegionPicker.cpp
    ui/RegionPicker.h
    ui/ReplayPlayer.cpp
    ui/ReplayPlayer.h
    ui/SettingsDialog.cpp
    ui/SettingsDialog.h
    ui/TextEditBox.cpp
    ui/TextEditBox.h
    ui/Toolbar.cpp
    ui/Toolbar.h
    ui/TrayIcon.cpp
    ui/TrayIcon.h)

if(WIN32)
    # Icon shown by Explorer and version details (Properties > Details).
    set(RECRAYON_ICON_FILE "${PROJECT_SOURCE_DIR}/packaging/windows/recrayon.ico")
    configure_file("${PROJECT_SOURCE_DIR}/cmake/recrayon.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/recrayon.rc" @ONLY)
    target_sources(recrayon PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/generated/recrayon.rc"
        platform/windows/WindowsGlobalHotkeys.cpp
        platform/windows/WindowsGlobalHotkeys.h)
    target_link_libraries(recrayon PRIVATE user32 gdi32 dwmapi)
endif()

if(RECRAYON_ENABLE_RECORDING AND Qt6Multimedia_FOUND)
    target_sources(recrayon PRIVATE
        capture/ComposedRecorder.cpp
        capture/ComposedRecorder.h
        capture/RecordingSession.cpp
        capture/RecordingSession.h
        capture/ScreenRecorder.cpp
        capture/ScreenRecorder.h)
    target_link_libraries(recrayon PRIVATE Qt6::Multimedia)
    target_compile_definitions(recrayon PRIVATE RECRAYON_HAS_RECORDING=1)
else()
    target_compile_definitions(recrayon PRIVATE RECRAYON_HAS_RECORDING=0)
endif()

target_include_directories(recrayon PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(recrayon PRIVATE recrayon_core Qt6::Widgets)
recrayon_set_warnings(recrayon)

set_target_properties(recrayon PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "Recrayon"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.recrayon.Recrayon"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")

if(APPLE)
    # Recrayon.app with its own Info.plist (microphone permission text, minimum macOS version)
    # and icon (packaging/macos/recrayon.icns, the same images as the Windows .ico).
    set(RECRAYON_MACOS_ICON "${PROJECT_SOURCE_DIR}/packaging/macos/recrayon.icns")
    set_source_files_properties("${RECRAYON_MACOS_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    target_sources(recrayon PRIVATE "${RECRAYON_MACOS_ICON}")
    set_target_properties(recrayon PROPERTIES
        OUTPUT_NAME "Recrayon"
        MACOSX_BUNDLE_ICON_FILE "recrayon.icns"
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/packaging/macos/Info.plist.in")
endif()

# ---- Translations ------------------------------------------------------------
# Source strings are English. translations/recrayon_<lang>.ts are updated from the code with
# `cmake --build --preset <preset> --target update_translations` and compiled to .qm at build
# time. The .qm files, plus Qt's own translations for those languages, are embedded under
# ":/i18n" (see app/Translations.cpp), so nothing has to be deployed next to the executable.
set(RECRAYON_LANGUAGES es)
if(Qt6LinguistTools_FOUND)
    set(recrayon_ts_files "")
    foreach(language IN LISTS RECRAYON_LANGUAGES)
        list(APPEND recrayon_ts_files "${PROJECT_SOURCE_DIR}/translations/recrayon_${language}.ts")
    endforeach()
    qt_add_lupdate(TS_FILES ${recrayon_ts_files}
        SOURCE_TARGETS recrayon recrayon_core
        OPTIONS -no-obsolete -locations none)
    qt_add_lrelease(TS_FILES ${recrayon_ts_files} QM_FILES_OUTPUT_VARIABLE recrayon_qm_files)
    qt_add_resources(recrayon "recrayon_translations"
        PREFIX "/i18n"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${recrayon_qm_files})

    set(qt_translations_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}")
    set(qt_qm_files "")
    foreach(language IN LISTS RECRAYON_LANGUAGES)
        foreach(module IN ITEMS qtbase qtmultimedia)
            if(EXISTS "${qt_translations_dir}/${module}_${language}.qm")
                list(APPEND qt_qm_files "${qt_translations_dir}/${module}_${language}.qm")
            endif()
        endforeach()
    endforeach()
    if(qt_qm_files)
        qt_add_resources(recrayon "qt_translations"
            PREFIX "/i18n"
            BASE "${qt_translations_dir}"
            FILES ${qt_qm_files})
    endif()
endif()

# ---- Run from the build tree (Windows) ---------------------------------------
# windeployqt copies the Qt DLLs, plugins (platforms/qwindows.dll, ...) and, for MinGW, the
# compiler runtime into the build directory, so build/<preset>/recrayon.exe starts directly.
if(WIN32 AND RECRAYON_DEPLOY_ON_BUILD AND TARGET Qt6::windeployqt)
    add_custom_command(TARGET recrayon POST_BUILD
        COMMAND Qt6::windeployqt
            --no-translations # embedded in the executable
            --no-system-d3d-compiler
            --no-system-dxc-compiler
            --no-opengl-sw
            --verbose 0
            "$<TARGET_FILE:recrayon>"
        COMMENT "Deploying Qt runtime next to recrayon.exe"
        VERBATIM)
endif()

# ---- Install / deploy --------------------------------------------------------
# Recrayon's license and the licenses of the bundled Qt / FFmpeg (LGPL) travel with every package.
set(RECRAYON_LICENSE_FILES
    "${PROJECT_SOURCE_DIR}/LICENSE"
    "${PROJECT_SOURCE_DIR}/THIRD-PARTY-NOTICES.md"
    "${PROJECT_SOURCE_DIR}/packaging/licenses/LGPL-3.0.txt"
    "${PROJECT_SOURCE_DIR}/packaging/licenses/GPL-3.0.txt")

if(WIN32)
    # Flat layout, like the build tree: recrayon.exe, the Qt DLLs and the plugin folders side by
    # side. Qt's translations are embedded in the executable (see Translations above), and the
    # software OpenGL / D3D compiler fallbacks are not used by this app (~20 MB less).
    install(TARGETS recrayon RUNTIME DESTINATION .)
    install(FILES ${RECRAYON_LICENSE_FILES} DESTINATION licenses)
    qt_generate_deploy_script(
        TARGET recrayon
        OUTPUT_SCRIPT recrayon_deploy_script
        CONTENT "
qt_deploy_runtime_dependencies(
    EXECUTABLE \"$<TARGET_FILE_NAME:recrayon>\"
    BIN_DIR .
    PLUGINS_DIR .
    NO_TRANSLATIONS
    DEPLOY_TOOL_OPTIONS --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler)
")
    install(SCRIPT "${recrayon_deploy_script}")
    include(WindowsInstaller)
elseif(APPLE)
    # Recrayon.app at the root of the prefix; macdeployqt copies the Qt frameworks and plugins
    # into the bundle. Packaged as a .dmg by CPack (see cmake/MacPackaging.cmake).
    install(TARGETS recrayon BUNDLE DESTINATION .)
    install(FILES ${RECRAYON_LICENSE_FILES} DESTINATION Recrayon.app/Contents/Resources/licenses)
    qt_generate_deploy_app_script(
        TARGET recrayon
        OUTPUT_SCRIPT recrayon_deploy_script
        NO_TRANSLATIONS)
    install(SCRIPT "${recrayon_deploy_script}")
    include(MacPackaging)
else()
    # Self-contained directory <prefix>/lib/recrayon (executable, qt.conf, Qt libraries in lib/
    # and plugins/), so a /usr install does not mix our Qt with the system's. <prefix>/bin/recrayon
    # is a symlink to it. Desktop entry, icon, .deb and AppImage: cmake/LinuxPackaging.cmake.
    install(TARGETS recrayon RUNTIME DESTINATION lib/recrayon)
    # Component "desktop" too, so the AppImage carries them (see cmake/LinuxPackaging.cmake).
    install(FILES ${RECRAYON_LICENSE_FILES} DESTINATION share/doc/recrayon COMPONENT desktop)
    set_target_properties(recrayon PROPERTIES INSTALL_RPATH "$ORIGIN/lib")
    # The QT_DEPLOY_*_DIR variables (not only the arguments) decide where qt.conf goes and how
    # plugin RPATHs are fixed. The installed executable's RPATH points to the still empty lib/, so
    # the dependency scan would miss the Qt libraries it links; the build-tree executable (RPATH
    # to the Qt installation) is scanned too, which only collects its dependencies.
    qt_generate_deploy_script(
        TARGET recrayon
        OUTPUT_SCRIPT recrayon_deploy_script
        CONTENT "
set(QT_DEPLOY_BIN_DIR lib/recrayon)
set(QT_DEPLOY_LIB_DIR lib/recrayon/lib)
set(QT_DEPLOY_PLUGINS_DIR lib/recrayon/plugins)
qt_deploy_runtime_dependencies(
    EXECUTABLE \"lib/recrayon/$<TARGET_FILE_NAME:recrayon>\"
    ADDITIONAL_EXECUTABLES \"$<TARGET_FILE:recrayon>\"
    BIN_DIR lib/recrayon
    LIB_DIR lib/recrayon/lib
    PLUGINS_DIR lib/recrayon/plugins
    GENERATE_QT_CONF
    NO_TRANSLATIONS)
")
    install(SCRIPT "${recrayon_deploy_script}")
    include(LinuxPackaging)
endif()
