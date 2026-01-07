cmake_minimum_required(VERSION 3.5)

project(qtcompleteapp VERSION 0.1 LANGUAGES CXX)

# Find Qt6 with GuiPrivate before adding subdirectories that need it
find_package(Qt6 COMPONENTS Core Gui GuiPrivate Widgets REQUIRED)

# Advanced Docking System (build as static library)
set(BUILD_STATIC ON CACHE BOOL "Build ADS as static library" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Don't build ADS examples" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/ads)
# set_target_properties(qtadvanceddocking-qt6 PROPERTIES BUILD_STATIC TRUE)

# Node Graph
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/nodegraph)

# 3D Viewer
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/viewer3d)

# 3D Viewer
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/colorpicker)

# Main App
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/texturelab)

