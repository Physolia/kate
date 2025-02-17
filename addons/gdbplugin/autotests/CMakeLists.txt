if(WIN32)
    return()
endif()

include(ECMMarkAsTest)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

add_executable(gdbplugin_test_gdbmi
    test_gdbmi.cpp
    ../gdbmi/tokens.cpp
    ../gdbmi/parser.cpp
)
target_include_directories(gdbplugin_test_gdbmi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(gdbplugin_test_gdbmi
  PRIVATE
    KF6::I18n
    KF6::TextEditor
    Qt::Test
)
add_test(NAME plugin-gdb_test_gdbmi COMMAND gdbplugin_test_gdbmi ${OFFSCREEN_QPA})
ecm_mark_as_test(gdbplugin_test_gdbmi)


add_executable(gdbplugin_test_gdbbackend
    test_gdbbackend.cpp
    ../gdbbackend.cpp
    ../backendinterface.cpp
    ../gdbvariableparser.cpp
    ../advanced_settings.cpp
    ../dap/entities.cpp
    ../gdbmi/tokens.cpp
    ../gdbmi/parser.cpp
)
target_include_directories(gdbplugin_test_gdbbackend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(
  gdbplugin_test_gdbbackend
  PRIVATE
    kateprivate
    KF6::I18n
    KF6::TextEditor
    Qt::Test
)
ki18n_wrap_ui(gdbplugin_test_gdbbackend ${CMAKE_CURRENT_SOURCE_DIR}/../advanced_settings.ui)
add_test(NAME plugin-gdb_test_gdbbackend COMMAND gdbplugin_test_gdbbackend ${OFFSCREEN_QPA})
ecm_mark_as_test(gdbplugin_test_gdbbackend)


add_executable(gdbplugin_test_gdbvariableparser
    test_gdbvariableparser.cpp
    ../gdbvariableparser.cpp
    ../dap/entities.cpp
)
target_include_directories(gdbplugin_test_gdbvariableparser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(
  gdbplugin_test_gdbvariableparser
  PRIVATE
    kateprivate
    KF6::I18n
    KF6::TextEditor
    Qt::Test
)
add_test(NAME plugin-gdb_test_gdbvariableparse COMMAND gdbplugin_test_gdbvariableparser ${OFFSCREEN_QPA})
ecm_mark_as_test(gdbplugin_test_gdbvariableparser)

if(ENABLE_PCH)
    target_precompile_headers(gdbplugin_test_gdbmi REUSE_FROM katepch_tests)
    target_precompile_headers(gdbplugin_test_gdbbackend REUSE_FROM katepch_tests)
    target_precompile_headers(gdbplugin_test_gdbvariableparser REUSE_FROM katepch_tests)
endif()
