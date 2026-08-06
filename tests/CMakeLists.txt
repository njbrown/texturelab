cmake_minimum_required(VERSION 3.10)

# QtTest rather than gtest or catch2: Qt is already a hard dependency, so this
# adds no third-party code to vendor, update, or explain.

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

# Each test file becomes its own binary, so one crashing suite can't take the
# others' results with it.
function(texturelab_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE
        catalog
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

texturelab_add_test(tst_database)
texturelab_add_test(tst_catalogindex)
texturelab_add_test(tst_thumbnailcache)
# QImage/QPixmap round-trip needs the GUI module.
target_link_libraries(tst_thumbnailcache PRIVATE Qt${QT_VERSION_MAJOR}::Gui)

# The launcher's model is deliberately free of app dependencies — Qt plus the
# catalog and nothing else — so its .cpp can be compiled straight into a test
# without dragging in the node graph or an OpenGL context.
texturelab_add_test(tst_texturelistmodel)
target_sources(tst_texturelistmodel PRIVATE
    ${CMAKE_SOURCE_DIR}/src/texturelab/launcher/texturelistmodel.cpp)
target_include_directories(tst_texturelistmodel PRIVATE
    ${CMAKE_SOURCE_DIR}/src/texturelab/launcher)
target_link_libraries(tst_texturelistmodel PRIVATE Qt${QT_VERSION_MAJOR}::Gui)
