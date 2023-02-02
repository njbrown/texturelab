cmake_minimum_required(VERSION 3.5)

project(qtcompleteapp VERSION 0.1 LANGUAGES CXX)

# Advanced Docking System
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/ads)
set_target_properties(qtadvanceddocking PROPERTIES BUILD_STATIC TRUE)

# Node Graph
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/nodegraph)

# 3D Viewer
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/viewer3d)

# 3D Viewer
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/colorpicker)

# Main App
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/texturelab)

