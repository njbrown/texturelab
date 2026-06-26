execute_process(
    COMMAND git -C "${SOURCE_DIR}" rev-parse --short HEAD
    OUTPUT_VARIABLE TEXTURELAB_BUILD_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT TEXTURELAB_BUILD_HASH)
    set(TEXTURELAB_BUILD_HASH "unknown")
endif()
configure_file("${SOURCE_DIR}/version.h.in" "${BINARY_DIR}/version.h" @ONLY)
