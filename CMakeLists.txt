cmake_minimum_required(VERSION 3.10)

project(qtcompleteapp VERSION 0.1 LANGUAGES CXX)

# sentry-native (Crashpad backend for out-of-process crash capture)
include(FetchContent)
FetchContent_Declare(
  sentry
  GIT_REPOSITORY https://github.com/getsentry/sentry-native.git
  GIT_TAG        0.7.20
)
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
if(WIN32)
  set(SENTRY_BACKEND "crashpad" CACHE STRING "" FORCE)
  set(SENTRY_TRANSPORT "winhttp" CACHE STRING "" FORCE)
elseif(APPLE)
  # Crashpad: out-of-process minidumps with full thread stacks (symbolicated via
  # the uploaded dSYM), same as Win/Linux. The inproc backend delivered crashes
  # but with no stack frames on Apple Silicon, so reports were unsymbolicatable.
  set(SENTRY_BACKEND "crashpad" CACHE STRING "" FORCE)
  set(SENTRY_TRANSPORT "curl" CACHE STRING "" FORCE)
else()
  set(SENTRY_BACKEND "crashpad" CACHE STRING "" FORCE)
  set(SENTRY_TRANSPORT "curl" CACHE STRING "" FORCE)
endif()
FetchContent_MakeAvailable(sentry)

# Find Qt6 with GuiPrivate before adding subdirectories that need it
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# GuiPrivate is needed for ADS on Qt6 (for qpa/qplatformnativeinterface.h)
if(Qt6_VERSION VERSION_GREATER_EQUAL "6.0.0")
    find_package(Qt6 COMPONENTS GuiPrivate QUIET)
endif()

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

