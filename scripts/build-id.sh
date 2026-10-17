#!/usr/bin/env bash
# Prints the build identity of a commit, worked out exactly the way the app
# (GetGitHash.cmake + src/texturelab/CMakeLists.txt) and the CI workflows do,
# so it matches the About box, the artifact names and what texturelab.io wants
# when publishing a release.
#
#   scripts/build-id.sh                # HEAD
#   scripts/build-id.sh 039faa9        # any commit, tag or branch
#   scripts/build-id.sh --field build  # one value only, for scripting
#
# Fields: identifier (0.4.0-beta-039faa9), version, channel, build, commit,
# tag (v-prefixed identifier, as used in artifact file names).
#
# Version and channel are read from that commit's CMakeLists.txt, not the
# working tree. The build number is the commit count, so it needs full history:
# in a shallow clone it comes out wrong, and this refuses to guess.

set -euo pipefail

FIELD=""
REF="HEAD"

while [ $# -gt 0 ]; do
    case "$1" in
        --field)
            FIELD=${2:?--field needs a name}
            shift 2
            ;;
        -h|--help)
            sed -n '2,17p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        -*)
            echo "unknown option: $1" >&2
            exit 2
            ;;
        *)
            REF=$1
            shift
            ;;
    esac
done

ROOT=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)

if [ "$(git -C "$ROOT" rev-parse --is-shallow-repository)" = "true" ]; then
    echo "error: shallow clone, so the commit count (build number) would be wrong; run 'git fetch --unshallow'" >&2
    exit 1
fi

COMMIT_FULL=$(git -C "$ROOT" rev-parse --verify --quiet "${REF}^{commit}") || {
    echo "error: '$REF' is not a commit" >&2
    exit 1
}

CMAKE=$(git -C "$ROOT" show "${COMMIT_FULL}:src/texturelab/CMakeLists.txt" 2>/dev/null) || {
    echo "error: no src/texturelab/CMakeLists.txt at $REF (older than the Qt app?)" >&2
    exit 1
}
VERSION=$(grep -oP 'project\(texturelab VERSION \K[0-9.]+' <<<"$CMAKE" || true)
CHANNEL=$(grep -oP 'set\(TEXTURELAB_CHANNEL "\K[^"]*' <<<"$CMAKE" || true)

if [ -z "$VERSION" ]; then
    echo "error: no project() version in src/texturelab/CMakeLists.txt at $REF" >&2
    exit 1
fi
if [ -z "$CHANNEL" ]; then
    # Commits from before the channel existed carried it as a version suffix.
    SUFFIX=$(grep -oP 'set\(TEXTURELAB_VERSION_SUFFIX "\K[^"]*' <<<"$CMAKE" || true)
    case "$SUFFIX" in
        "") CHANNEL=stable ;;
        -beta*) CHANNEL=beta ;;
        *)
            echo "error: can't tell the channel at $REF (version suffix '$SUFFIX')" >&2
            exit 1
            ;;
    esac
fi

# --short=7 and rev-list --count: the same commands GetGitHash.cmake runs.
COMMIT=$(git -C "$ROOT" rev-parse --short=7 "$COMMIT_FULL")
BUILD=$(git -C "$ROOT" rev-list --count "$COMMIT_FULL")
IDENTIFIER="${VERSION}-${CHANNEL}-${COMMIT}"

case "$FIELD" in
    "")
        printf 'identifier  %s\n' "$IDENTIFIER"
        printf 'version     %s\n' "$VERSION"
        printf 'channel     %s\n' "$CHANNEL"
        printf 'build       %s\n' "$BUILD"
        printf 'commit      %s\n' "$COMMIT"
        printf 'tag         v%s\n' "$IDENTIFIER"
        ;;
    identifier) echo "$IDENTIFIER" ;;
    version) echo "$VERSION" ;;
    channel) echo "$CHANNEL" ;;
    build) echo "$BUILD" ;;
    commit) echo "$COMMIT" ;;
    tag) echo "v$IDENTIFIER" ;;
    *)
        echo "unknown field: $FIELD (identifier, version, channel, build, commit, tag)" >&2
        exit 2
        ;;
esac
