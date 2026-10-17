# Short hash and commit count of the checked-out tree, written to version.h.
#
# The commit count is the build number: it strictly increases along master, so
# two builds of the same version (every beta shares one) still have an order.
# The hash only identifies a build; it is never compared. --short=7 pins the
# length so a shallow CI checkout and a full clone name a commit the same way.
execute_process(
    COMMAND git -C "${SOURCE_DIR}" rev-parse --short=7 HEAD
    OUTPUT_VARIABLE TEXTURELAB_BUILD_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT TEXTURELAB_BUILD_HASH)
    set(TEXTURELAB_BUILD_HASH "unknown")
endif()

# Needs full history: a shallow clone counts only the commits it fetched. Every
# packaging job checks out with fetch-depth: 0.
execute_process(
    COMMAND git -C "${SOURCE_DIR}" rev-list --count HEAD
    OUTPUT_VARIABLE TEXTURELAB_BUILD_NUMBER
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT TEXTURELAB_BUILD_NUMBER MATCHES "^[0-9]+$")
    set(TEXTURELAB_BUILD_NUMBER 0)
endif()

configure_file("${SOURCE_DIR}/version.h.in" "${BINARY_DIR}/version.h" @ONLY)
