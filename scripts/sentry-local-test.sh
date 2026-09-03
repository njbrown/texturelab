#!/usr/bin/env bash
# Local end-to-end test for Sentry crash symbolication (Linux).
#
# Mirrors what CI does, then triggers a real crash so you can confirm the
# stack trace is readable in sentry.io.
#
# Requires: sentry-cli on PATH, and these env vars:
#   SENTRY_AUTH_TOKEN  – an auth token with project:write / project:releases
#   SENTRY_ORG         – your Sentry org slug
#   SENTRY_PROJECT     – your Sentry project slug
#
# Usage:
#   SENTRY_AUTH_TOKEN=xxx SENTRY_ORG=xxx SENTRY_PROJECT=xxx \
#     ./scripts/sentry-local-test.sh [build-dir]
set -euo pipefail

BUILD_DIR="${1:-build-sentry-test}"
BIN="$BUILD_DIR/src/texturelab/texturelab"

: "${SENTRY_AUTH_TOKEN:?set SENTRY_AUTH_TOKEN}"
: "${SENTRY_ORG:?set SENTRY_ORG}"
: "${SENTRY_PROJECT:?set SENTRY_PROJECT}"

if [ ! -f "$BIN" ]; then
  echo "!! $BIN not found. Build first:"
  echo "   cmake --build $BUILD_DIR --target texturelab --parallel \$(nproc)"
  exit 1
fi

echo "== 1) Inspecting DIF of the freshly built binary =="
sentry-cli debug-files check "$BIN"

echo
echo "== 2) Uploading FULL unstripped binary (debug + unwind + sources) =="
sentry-cli debug-files upload --include-sources "$BIN"

echo
echo "== 3) Stripping the shipped copy (build-id / Debug ID is preserved) =="
strip "$BIN"
sentry-cli debug-files check "$BIN"   # should still show a matching Debug ID

echo
echo "== 4) Triggering a deliberate crash so Crashpad uploads a minidump =="
# Wipe any stale crash DB so we know the minidump is from this run.
rm -rf "$HOME/.local/share/texturelab/texturelab/sentry" 2>/dev/null || true
set +e
"$BIN" --sentry-crash-test
echo "   app exited with code $? (a crash is expected)"
set -e

echo
echo "== Done =="
echo "Crashpad uploads the minidump in the background. Open your Sentry project:"
echo "  https://$SENTRY_ORG.sentry.io/issues/"
echo "You should see a new crash whose stack trace includes 'sentryCrashTest'"
echo "and 'main' with file/line info. If the frames are symbolicated, the fix works."
