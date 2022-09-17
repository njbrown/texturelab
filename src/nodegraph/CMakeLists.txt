
cmake_minimum_required(VERSION 3.5)


find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.5 COMPONENTS Core Gui Widgets REQUIRED)
if (UNIX AND NOT APPLE)
    include_directories(${Qt${QT_VERSION_MAJOR}Gui_PRIVATE_INCLUDE_DIRS})
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(NODEGRAPH_SRCS
    # main.cpp
    nodegraph.cpp
)
set(NODEGRAPH_HEADERS
    nodegraph.h
)

# library
add_library(nodegraph STATIC ${NODEGRAPH_SRCS} ${NODEGRAPH_HEADERS})
target_link_libraries(nodegraph PUBLIC Qt${QT_VERSION_MAJOR}::Core 
                                               Qt${QT_VERSION_MAJOR}::Gui 
                                               Qt${QT_VERSION_MAJOR}::Widgets)


set_target_properties(nodegraph PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    EXPORT_NAME "Nodegraph Library"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(QT_VERSION_MAJOR STREQUAL "5")
    set_target_properties(nodegraph PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON)
elseif(QT_VERSION_MAJOR STREQUAL "6")
    set_target_properties(nodegraph PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON)
endif()

# EXE for testing purposes
add_executable(nodegraph_app
            ${NODEGRAPH_HEADERS}
            ${NODEGRAPH_SRCS}
            main.cpp
        )
target_link_libraries(nodegraph_app PRIVATE Qt${QT_VERSION_MAJOR}::Core
                                            Qt${QT_VERSION_MAJOR}::Gui
                                            Qt${QT_VERSION_MAJOR}::Widgets)


set_target_properties(nodegraph_app PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    EXPORT_NAME "Nodegraph App"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(QT_VERSION_MAJOR STREQUAL "5")
    set_target_properties(nodegraph_app PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON)
elseif(QT_VERSION_MAJOR STREQUAL "6")
    set_target_properties(nodegraph_app PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON)
endif()

# if(QT_VERSION_MAJOR EQUAL 6)
#     qt_finalize_executable(qtapp)
# endif()