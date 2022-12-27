
cmake_minimum_required(VERSION 3.5)


find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.5 COMPONENTS Core Gui Widgets OpenGLWidgets REQUIRED)
if (UNIX AND NOT APPLE)
    include_directories(${Qt${QT_VERSION_MAJOR}Gui_PRIVATE_INCLUDE_DIRS})
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SRCS
    main.cpp
    viewer3d.cpp
    iblsampler.cpp
    shadercache.cpp
    gltf/gltf.cpp
    geometry/sphere.cpp
    renderer/renderer.cpp
)
set(HEADERS
    viewer3d.h
    iblsampler.h
    shadercache.h
    geometry/geometry.h
    renderer/renderer.h
    gltf/json.hpp
    gltf/tiny_gltf.h
    gltf/stb_image.h
)

set(RESOURCES
    ./assets.qrc)

# library
add_library(viewer3d STATIC ${SRCS} ${HEADERS})
target_link_libraries(viewer3d PRIVATE Qt${QT_VERSION_MAJOR}::Core 
                                        Qt${QT_VERSION_MAJOR}::Gui 
                                        Qt${QT_VERSION_MAJOR}::OpenGLWidgets 
                                        Qt${QT_VERSION_MAJOR}::Widgets)


set_target_properties(viewer3d PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    EXPORT_NAME "3d Viewer"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(QT_VERSION_MAJOR STREQUAL "5")
    set_target_properties(viewer3d PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON)
elseif(QT_VERSION_MAJOR STREQUAL "6")
    set_target_properties(viewer3d PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON)
endif()

# EXE for testing purposes
add_executable(viewer3d_app
            ${HEADERS}
            ${SRCS}
            main.cpp
            ${RESOURCES}
        )
target_link_libraries(viewer3d_app PRIVATE Qt${QT_VERSION_MAJOR}::Core
                                            Qt${QT_VERSION_MAJOR}::Gui
                                            Qt${QT_VERSION_MAJOR}::OpenGLWidgets 
                                            Qt${QT_VERSION_MAJOR}::Widgets)


set_target_properties(viewer3d_app PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    EXPORT_NAME "Viewer Sample App"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(QT_VERSION_MAJOR STREQUAL "5")
    set_target_properties(viewer3d_app PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON)
elseif(QT_VERSION_MAJOR STREQUAL "6")
    set_target_properties(viewer3d_app PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON)
endif()