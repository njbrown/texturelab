cmake_minimum_required(VERSION 3.10)

project(texturelab VERSION 0.4.0 LANGUAGES CXX)

# Optional pre-release suffix appended to the numeric project() version above to
# form the full app version (e.g. "0.4.0" + "-beta" = "0.4.0-beta"); leave empty
# for a final release. The full string is the in-app version / Sentry release
# (TEXTURELAB_VERSION compile def below) and is reconstructed from these same two
# lines by the build workflow (.github/workflows/build.yml) for artifact names.
set(TEXTURELAB_VERSION_SUFFIX "-beta")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets OpenGL)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(OpenGL REQUIRED)

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

set(LIBRARYV3
    ./libraries/v3/ambientocclusion.cpp
    ./libraries/v3/bevelv2.cpp
    ./libraries/v3/curvenode.cpp
    ./libraries/v3/curvature.cpp
    ./libraries/v3/maskedblur.cpp
    ./libraries/v3/rays.cpp
    ./libraries/v3/swirl.cpp
    ./libraries/v3/floodfillv2.cpp
    ./libraries/v3/floodfillv2tocolor.cpp
    ./libraries/v3/floodfillv2torandomcolor.cpp
    ./libraries/v3/floodfillv2torandomintensity.cpp
    ./libraries/v3/floodfillv2tobbox.cpp
    ./libraries/v3/floodfillv2togradient.cpp
    ./libraries/v3/floodfillv2sampler.cpp
    # Phase 1 — Filters / Color
    ./libraries/v3/blendv3.cpp
    ./libraries/v3/edgedetect.cpp
    ./libraries/v3/highpass.cpp
    ./libraries/v3/emboss.cpp
    ./libraries/v3/vibrance.cpp
    ./libraries/v3/colortomask.cpp
    ./libraries/v3/setalpha.cpp
    ./libraries/v3/toongradient.cpp
    ./libraries/v3/autolevels.cpp
    # Phase 2 — Generators
    ./libraries/v3/bricks2.cpp
    ./libraries/v3/directionalscratches.cpp
    ./libraries/v3/roughgrain.cpp
    ./libraries/v3/voronoifractal.cpp
    ./libraries/v3/truchet.cpp
    ./libraries/v3/fbmdomainwarp.cpp
    ./libraries/v3/perlinnoise.cpp
    ./libraries/v3/perlinnoise3d.cpp
    ./libraries/v3/cellv3.cpp
    ./libraries/v3/linecellv3.cpp
    ./libraries/v3/solidcellv3.cpp
    # Phase 3 — Multi-pass
    ./libraries/v3/blurhq.cpp
    ./libraries/v3/distancetransform.cpp
    ./libraries/v3/spread.cpp
    ./libraries/v3/heightblend.cpp
    ./libraries/v3/makeittile.cpp
    ./libraries/v3/normalmapv3.cpp
)

set(PROJECT_SOURCES
        ./main.cpp
        ./telemetry.h
        ./telemetry.cpp
        ./mainwindow.cpp
        ./mainwindow.h
        ./clipboard.h
        ./clipboard.cpp
        ./exporter.h
        ./exporter.cpp
        ./utils.h
        ./curve.h
        ./curve.cpp
        ./models.h
        ./models.cpp
        ./project.h
        ./project.cpp
        ./props.h
        ./props.cpp
        ./libraries/libv2.h
        ./libraries/libv3.h
        ./libraries/library.h
        ./libraries/library.cpp
        ./libraries/libversion.h
        ./libraries/libversion.cpp
        ./libraries/libraryversionmigrator.h
        ./libraries/libraryversionmigrator.cpp
        ./widgets/graphwidget.h
        ./widgets/graphwidget.cpp
        ./widgets/librarywidget.h
        ./widgets/librarywidget.cpp
        ./widgets/nodesearchpopup.h
        ./widgets/nodesearchpopup.cpp
        ./widgets/properties/propertieswidget.h
        ./widgets/properties/propertieswidget.cpp
        ./widgets/properties/propwidgets.h
        ./widgets/properties/propwidgets.cpp
        ./widgets/properties/curvepropwidget.h
        ./widgets/properties/curvepropwidget.cpp
        ./widgets/properties/accordionwidget.h
        ./widgets/properties/accordionwidget.cpp
        ./widgets/view2dwidget.h
        ./widgets/view2dwidget.cpp
        ./widgets/view3dwidget.h
        ./widgets/view3dwidget.cpp
        ./undo/undocommands.h
        ./undo/undocommandids.h
        ./undo/addnodecommand.h
        ./undo/addnodecommand.cpp
        ./undo/deleteitemscommand.h
        ./undo/deleteitemscommand.cpp
        ./undo/addconnectioncommand.h
        ./undo/addconnectioncommand.cpp
        ./undo/removeconnectioncommand.h
        ./undo/removeconnectioncommand.cpp
        ./undo/moveitemscommand.h
        ./undo/moveitemscommand.cpp
        ./undo/propertychangecommand.h
        ./undo/propertychangecommand.cpp
        ./undo/randomseedchangecommand.h
        ./undo/randomseedchangecommand.cpp
        ./undo/addframecommand.h
        ./undo/addframecommand.cpp
        ./undo/addcommentcommand.h
        ./undo/addcommentcommand.cpp
        ./undo/editframecommand.h
        ./undo/editframecommand.cpp
        ./undo/editcommentcommand.h
        ./undo/editcommentcommand.cpp
        ./undo/pastecommand.h
        ./undo/pastecommand.cpp
        ./undo/texturechannelassigncommand.h
        ./undo/texturechannelassigncommand.cpp
        ./widgets/aboutdialog.h
        ./widgets/aboutdialog.cpp
        ./widgets/exportdialog.h
        ./widgets/exportdialog.cpp
        ./graphics/texturerenderer.h
        ./graphics/texturerenderer.cpp
        ./graphics/noderenderer.h
        ./graphics/noderenderer.cpp
        ./graphics/renderworker.h
        ./graphics/renderworker.cpp
        ${LIBRARYV1}
        ${LIBRARYV2}
        ${LIBRARYV3}
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
                                         OpenGL::GL
                                         qtadvanceddocking-qt6
                                         nodegraph
                                         viewer3d
                                         colorpicker
                                         sentry
                                         )
                                         
target_include_directories(texturelab PUBLIC
                            ../ads/src
                            ../nodegraph
                            ../viewer3d
                            ../colorpicker
                            )
# DSN is empty by default for local builds (Sentry init becomes a no-op)
if(NOT DEFINED TEXTURELAB_SENTRY_DSN)
    set(TEXTURELAB_SENTRY_DSN "")
endif()

target_compile_definitions(texturelab PRIVATE
    TEXTURELAB_VERSION="${PROJECT_VERSION}${TEXTURELAB_VERSION_SUFFIX}"
    TEXTURELAB_SENTRY_DSN="${TEXTURELAB_SENTRY_DSN}"
)

# Generate version.h (with git hash) at every build, not just configure time
add_custom_target(texturelab_version
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        -DBINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/GetGitHash.cmake"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/version.h"
    COMMENT "Generating version.h"
)
add_dependencies(texturelab texturelab_version)
target_include_directories(texturelab PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

# MSVC: deterministically emit a PDB and embed a CodeView record in the exe, in
# every config (incl. Release). Without the CodeView record the shipped exe has
# no debug_id, so Sentry cannot match any PDB to a crash minidump ("Unknown
# function" frames). Attaching to the target is reliable; injecting /Zi + /DEBUG
# via CMAKE_*_FLAGS_RELEASE on the command line was not (VS generator + Qt).
if(MSVC)
    target_compile_options(texturelab PRIVATE /Zi)
    target_link_options(texturelab PRIVATE /DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO)
endif()

# Copy crashpad_handler next to the executable so handler_path resolves at runtime
if(TARGET crashpad_handler)
    add_custom_command(TARGET texturelab POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:crashpad_handler>
            $<TARGET_FILE_DIR:texturelab>
        COMMENT "Copying crashpad_handler next to texturelab"
    )
endif()

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
