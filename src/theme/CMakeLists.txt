cmake_minimum_required(VERSION 3.10)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(THEME_SRCS
    theme.cpp
    qssbuilder.cpp
    thememanager.cpp
)
set(THEME_HEADERS
    tokens.h
    theme.h
    qssbuilder.h
    thememanager.h
)

add_library(theme STATIC ${THEME_SRCS} ${THEME_HEADERS})

target_link_libraries(theme PUBLIC Qt${QT_VERSION_MAJOR}::Core
                                   Qt${QT_VERSION_MAJOR}::Gui
                                   Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(theme PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(theme PROPERTIES
    AUTOMOC ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
