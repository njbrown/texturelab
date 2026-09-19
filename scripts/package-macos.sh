#!/usr/bin/env bash
#
# Bundles Qt into the built texturelab.app, optionally signs it, and produces a
# DMG in dist/. Used by .github/workflows/release.yml; runnable locally too.
#
#   scripts/package-macos.sh <version-tag> [build-dir]
#   e.g. scripts/package-macos.sh v0.4.0-beta build
#
# Signing is optional: set MAC_SIGN_ID to a "Developer ID Application: ..."
# identity to sign the bundle and the DMG. Notarization is a separate step
# (see the workflow) and must run after this script.
# macdeployqt is found via qmake on PATH, or $QTDIR / $MACDEPLOYQT.
set -euo pipefail

TAG=${1:?usage: package-macos.sh <version-tag> [build-dir]}
BUILD=${2:-build}
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"

APP="$BUILD/src/texturelab/texturelab.app"
[ -d "$APP" ] || { echo "error: $APP not found — build first" >&2; exit 1; }

if [ -z "${MACDEPLOYQT:-}" ]; then
    if [ -n "${QTDIR:-}" ]; then
        MACDEPLOYQT="$QTDIR/bin/macdeployqt"
    else
        QMAKE_BIN=$(command -v qmake6 || command -v qmake) || {
            echo "error: set QTDIR/MACDEPLOYQT or put qmake on PATH" >&2; exit 1; }
        MACDEPLOYQT="$(dirname "$QMAKE_BIN")/macdeployqt"
    fi
fi
[ -x "$MACDEPLOYQT" ] || { echo "error: macdeployqt not found at $MACDEPLOYQT" >&2; exit 1; }

# crashpad_handler is copied next to the binary at build time; it must end up
# inside the bundle or crash reporting silently does nothing.
[ -f "$APP/Contents/MacOS/crashpad_handler" ] || {
    echo "error: crashpad_handler missing from the bundle" >&2; exit 1; }

"$MACDEPLOYQT" "$APP"

# Ship only the SQLite driver, matching the Linux package.
find "$APP/Contents/PlugIns/sqldrivers" -name 'libqsql*.dylib' \
    ! -name 'libqsqlite.dylib' -delete 2>/dev/null || true
cp LICENSE "$APP/Contents/Resources/"

if [ -n "${MAC_SIGN_ID:-}" ]; then
    # Hardened runtime + a secure timestamp are required for notarization.
    # --deep also covers the bundled Qt frameworks, plugins and crashpad_handler.
    codesign --force --deep --options runtime --timestamp --sign "$MAC_SIGN_ID" "$APP"
    codesign --verify --deep --strict --verbose=2 "$APP"
else
    echo "warning: MAC_SIGN_ID not set — building an UNSIGNED app (Gatekeeper will block it)" >&2
fi

STAGE="$BUILD/dmg-stage"
rm -rf "$STAGE"
mkdir -p "$STAGE" dist
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications" # drag-to-install layout

OUT="dist/texturelab-mac-${TAG}-universal.dmg"
rm -f "$OUT"
hdiutil create -volname "TextureLab" -srcfolder "$STAGE" -format UDZO -ov "$OUT"
if [ -n "${MAC_SIGN_ID:-}" ]; then
    codesign --force --timestamp --sign "$MAC_SIGN_ID" "$OUT"
fi

echo "packaged: $OUT"
