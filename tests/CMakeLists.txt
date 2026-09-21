find_package(Qt6 REQUIRED COMPONENTS Test)

# recrayon_add_test(<name>)
#
# Builds tests/unit/<name>.cpp as a QtTest executable linked against recrayon_core and
# registers it with CTest. Tests run with the offscreen platform so CI needs no display.
function(recrayon_add_test name)
    qt_add_executable(${name} "unit/${name}.cpp")
    target_link_libraries(${name} PRIVATE recrayon_core Qt6::Test)
    recrayon_set_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
    # The build directory may hold a deployed platforms/ folder without the offscreen plugin
    # (RECRAYON_DEPLOY_ON_BUILD), so point Qt at the plugins of the Qt installation.
    set_tests_properties(${name} PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}/platforms")
endfunction()

recrayon_add_test(tst_desktoplayout)
recrayon_add_test(tst_document)
recrayon_add_test(tst_items)
recrayon_add_test(tst_replay)
recrayon_add_test(tst_settings)
recrayon_add_test(tst_text)
recrayon_add_test(tst_toolcontroller)
