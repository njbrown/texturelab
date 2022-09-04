cmake_minimum_required(VERSION 3.5)

project(texturelab VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        ./main.cpp
        ./mainwindow.cpp
        ./mainwindow.h
        ./models.h
        ./props.h
        ./props.cpp
)

set(PROJECT_RESOURCES
    ./assets.qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(texturelab
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${PROJECT_RESOURCES}
    )
else()
    add_executable(texturelab
        ${PROJECT_SOURCES}
        ${PROJECT_RESOURCES}
    )
endif()

target_link_libraries(texturelab PRIVATE Qt${QT_VERSION_MAJOR}::Widgets qtadvanceddocking)
target_include_directories(texturelab PUBLIC
                            ../ads/src
                            )

set_target_properties(texturelab PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER io.texturelab.app
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)


install(TARGETS texturelab
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(texturelab)
endif()
