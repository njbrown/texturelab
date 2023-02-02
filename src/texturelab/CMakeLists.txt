cmake_minimum_required(VERSION 3.5)

project(texturelab VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets OpenGL)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets)

set(LIBRARYV1
    ./libraries/v1/blend.cpp
    ./libraries/v1/brickgenerator.cpp
    ./libraries/v1/brightnesscontrast.cpp
    ./libraries/v1/cell.cpp
    ./libraries/v1/checkerboard.cpp
    ./libraries/v1/copy.cpp
    ./libraries/v1/color.cpp
    ./libraries/v1/directionalwarp.cpp
    ./libraries/v1/fractalnoise.cpp
    ./libraries/v1/gradient.cpp
    ./libraries/v1/gradientmap.cpp
    ./libraries/v1/heightshift.cpp
    ./libraries/v1/hexagon.cpp
    ./libraries/v1/invert.cpp
    ./libraries/v1/linecell.cpp
    ./libraries/v1/maprange.cpp
    ./libraries/v1/mask.cpp
    ./libraries/v1/mirror.cpp
    ./libraries/v1/normalmap.cpp
    ./libraries/v1/output.cpp
    ./libraries/v1/perlin3d.cpp
    ./libraries/v1/shapes.cpp
    ./libraries/v1/solidcell.cpp
    ./libraries/v1/splat.cpp
    ./libraries/v1/threshold.cpp
    ./libraries/v1/tile.cpp
    ./libraries/v1/transform2d.cpp
    ./libraries/v1/warp.cpp
    ./libraries/v1/wave.cpp
)

set(LIBRARYV2
    ./libraries/v2/advancesplatter.cpp
    ./libraries/v2/anisotropicblur.cpp
    ./libraries/v2/bevel.cpp
    ./libraries/v2/blur.cpp
    ./libraries/v2/capsule.cpp
    ./libraries/v2/cartesiantopolar.cpp
    ./libraries/v2/circularsplatter.cpp
    ./libraries/v2/clamp.cpp
    ./libraries/v2/combinenormals.cpp
    ./libraries/v2/directionalblur.cpp
    ./libraries/v2/directionalwarpv2.cpp
    ./libraries/v2/extractchannel.cpp
    ./libraries/v2/floodfill.cpp
    ./libraries/v2/floodfillsampler.cpp
    ./libraries/v2/floodfilltobbox.cpp
    ./libraries/v2/floodfilltocolor.cpp
    ./libraries/v2/floodfilltogradient.cpp
    ./libraries/v2/floodfilltorandomcolor.cpp
    ./libraries/v2/floodfilltorandomintensity.cpp
    ./libraries/v2/gradientdynamic.cpp
    ./libraries/v2/gradientnoise.cpp
    ./libraries/v2/gradientnoisefractalsum.cpp
    ./libraries/v2/grayscale.cpp
    ./libraries/v2/histogramscan.cpp
    ./libraries/v2/histogramselect.cpp
    ./libraries/v2/histogramshift.cpp
    ./libraries/v2/hsl.cpp
    ./libraries/v2/hslextract.cpp
    ./libraries/v2/imagenode.cpp
    ./libraries/v2/invertnormal.cpp
    ./libraries/v2/normalmapv2.cpp
    ./libraries/v2/polartocartesian.cpp
    ./libraries/v2/pow.cpp
    ./libraries/v2/quantize.cpp
    ./libraries/v2/rgbamerge.cpp
    ./libraries/v2/rgbashuffle.cpp
    ./libraries/v2/simplexnoisev2.cpp
    ./libraries/v2/skew.cpp
    ./libraries/v2/slopeblur.cpp
    ./libraries/v2/softflower.cpp
    ./libraries/v2/splatv2.cpp
    ./libraries/v2/star.cpp
    ./libraries/v2/stripes.cpp
    ./libraries/v2/tilesampler.cpp
    ./libraries/v2/transform2dv2.cpp
    ./libraries/v2/valuenoise.cpp
    ./libraries/v2/valuenoisefractalsum.cpp
    ./libraries/v2/warpv2.cpp
)

set(PROJECT_SOURCES
        ./main.cpp
        ./mainwindow.cpp
        ./mainwindow.h
        ./utils.h
        ./models.h
        ./models.cpp
        ./project.h
        ./project.cpp
        ./props.h
        ./props.cpp
        ./libraries/libv2.h
        ./libraries/libv2.h
        ./libraries/library.h
        ./libraries/library.cpp
        ./widgets/graphwidget.h
        ./widgets/graphwidget.cpp
        ./widgets/librarywidget.h
        ./widgets/librarywidget.cpp
        ./widgets/properties/propertieswidget.h
        ./widgets/properties/propertieswidget.cpp
        ./widgets/properties/propwidgets.h
        ./widgets/properties/propwidgets.cpp
        ./widgets/view2dwidget.h
        ./widgets/view2dwidget.cpp
        ./widgets/view3dwidget.h
        ./widgets/view3dwidget.cpp
        ./graphics/texturerenderer.h
        ./graphics/texturerenderer.cpp
        ${LIBRARYV1}
        ${LIBRARYV2}
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

# note: openglwidgets is qt6 only
target_link_libraries(texturelab PRIVATE Qt${QT_VERSION_MAJOR}::Widgets 
                                         Qt${QT_VERSION_MAJOR}::OpenGL 
                                         Qt${QT_VERSION_MAJOR}::OpenGLWidgets 
                                         qtadvanceddocking 
                                         nodegraph
                                         viewer3d)
                                         
target_include_directories(texturelab PUBLIC
                            ../ads/src
                            ../nodegraph
                            ../viewer3d
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
