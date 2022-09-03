cmake_minimum_required(VERSION 3.5)

project(qtcompleteapp VERSION 0.1 LANGUAGES CXX)

# Advanced Docking System
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/ads)
set_target_properties(qtadvanceddocking PROPERTIES BUILD_STATIC TRUE)

# Main App
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/texturelab)

