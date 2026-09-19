#!/usr/bin/env bash
#
# Packages an already-built texturelab into an AppImage in dist/.
# Used by .github/workflows/release.yml; runnable locally for the same result.
#
#   scripts/package-linux.sh <version-tag> [build-dir]
#   e.g. scripts/package-linux.sh v0.4.0-beta build
#
# Requires linuxdeploy + its qt plugin on PATH (override with $LINUXDEPLOY).
# Qt is found via qmake on PATH, or $QTDIR.
set -euo pipefail

TAG=${1:?usage: package-linux.sh <version-tag> [build-dir]}
BUILD=${2:-build}
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"

BIN="$BUILD/src/texturelab/texturelab"
[ -x "$BIN" ] || { echo "error: $BIN not found — build first" >&2; exit 1; }

LINUXDEPLOY=${LINUXDEPLOY:-linuxdeploy-x86_64.AppImage}
command -v "$LINUXDEPLOY" >/dev/null || { echo "error: $LINUXDEPLOY not on PATH" >&2; exit 1; }

# Qt: prefer an explicit $QTDIR, else derive it from qmake's location.
if [ -z "${QTDIR:-}" ]; then
    QMAKE_BIN=$(command -v qmake6 || command -v qmake) || {
        echo "error: set QTDIR or put qmake on PATH" >&2; exit 1; }
    QTDIR=$(dirname "$(dirname "$QMAKE_BIN")")
fi
export QMAKE="$QTDIR/bin/qmake"
export PATH="$QTDIR/bin:$PATH"
export LD_LIBRARY_PATH="$QTDIR/lib:${LD_LIBRARY_PATH:-}"

# linuxdeploy's qt plugin deploys *every* sqldriver it finds and hard-fails when
# one has an unresolvable dependency — Qt's libqsqlmimer.so needs a proprietary
# Mimer client that isn't installed. We only ever open QSQLITE
# (src/catalog/database.cpp), so drop the rest; that also avoids shipping the
# GPL libmysqlclient the MySQL driver drags in.
if [ -w "$QTDIR/plugins/sqldrivers" ]; then
    find "$QTDIR/plugins/sqldrivers" -name 'libqsql*.so' ! -name 'libqsqlite.so' -delete
fi

APPDIR="$BUILD/AppDir"
STAGE="$BUILD/appimage-stage"
rm -rf "$APPDIR" "$STAGE"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/doc/texturelab" "$STAGE" dist

# crashpad_handler ships next to the binary; without it crashes go unreported.
HANDLER=$(ls "$BUILD/src/texturelab/crashpad_handler" \
             "$BUILD/_deps/sentry-build/crashpad_build/handler/crashpad_handler" \
             2>/dev/null | head -1) || true
[ -n "$HANDLER" ] || { echo "error: crashpad_handler not found in $BUILD" >&2; exit 1; }
cp "$HANDLER" "$APPDIR/usr/bin/"
cp LICENSE "$APPDIR/usr/share/doc/texturelab/"

# The icon's basename must match the desktop entry's Icon= key (texturelab).
cp packaging/linux/texturelab.desktop "$STAGE/texturelab.desktop"
cp src/icons/logo.png "$STAGE/texturelab.png"

OUT="dist/texturelab-linux-${TAG}-x86_64.AppImage"
rm -f "$OUT"
# The appimage plugin writes to $OUTPUT, so we never have to guess the name.
APPIMAGE_EXTRACT_AND_RUN=${APPIMAGE_EXTRACT_AND_RUN:-1} \
DEPLOY_STDCXX=${DEPLOY_STDCXX:-1} \
OUTPUT="$OUT" \
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$BIN" \
    --desktop-file "$STAGE/texturelab.desktop" \
    --icon-file "$STAGE/texturelab.png" \
    --plugin qt \
    --output appimage

chmod +x "$OUT"
echo "packaged: $OUT"
