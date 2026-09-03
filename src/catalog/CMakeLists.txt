cmake_minimum_required(VERSION 3.10)

# The launcher's data layer: index.db (what textures exist) and thumbs.db
# (their cached previews). See LAUNCHER_PRD.md.
#
# Deliberately free of any dependency on the app's node graph or on Qt Widgets,
# so it builds and tests headless. The mapping between TextureChannel and this
# library's ChannelBit lives at the app boundary, not here.

project(catalog LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sets QT_VERSION_MAJOR, matching how the other subdirectories resolve Qt.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql)

add_library(catalog STATIC
    database.h
    database.cpp
    catalogindex.h
    catalogindex.cpp
    thumbnailcache.h
    thumbnailcache.cpp
    texturerecord.h
)

target_include_directories(catalog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Qt6::Sql pulls in the bundled QSQLITE driver plugin. The deploy tooling
# (linuxdeploy --plugin qt, windeployqt, macdeployqt) copies sqldrivers/
# automatically for any target that links it.
target_link_libraries(catalog PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

set_target_properties(catalog PROPERTIES POSITION_INDEPENDENT_CODE ON)
