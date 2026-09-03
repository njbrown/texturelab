#!/usr/bin/env bash
#
# Theme hygiene gate.
#
# The theme system (src/theme/ + resources/themes/*.json + resources/qss/*.qss.in)
# is the single source of truth for the app's look. This check fails the build if
# new *inline widget stylesheets* or *hardcoded QColor literals* creep into the
# UI / rendering code, which would bypass the theme and break --dev-theme
# hot-reload.
#
# Legitimate exceptions carry a "// theme-exempt: <reason>" marker on the same
# line (e.g. dynamic color-DATA swatches, or the one call that applies the
# composed theme sheet to the ADS dock manager). See UI_DESIGN_SYSTEM_PRD.md.
#
# Out of scope by design: src/theme (the system), src/ads (vendored submodule),
# src/colorpicker (a color-DATA widget lib), src/texturelab/libraries (node
# default *values*, not UI styling).

set -uo pipefail
cd "$(dirname "$0")/.."

# UI + rendering code that must stay theme-driven.
SCOPE=(
  "src/texturelab/widgets"
  "src/texturelab/mainwindow.cpp"
  "src/nodegraph/graph"
)

status=0

# Drop matches that live on a commented-out line (content after "file:line:"
# starts with //) and any line carrying the theme-exempt marker.
drop_noise() { grep -vE ':[0-9]+:[[:space:]]*//' | grep -v 'theme-exempt'; }

# 1) Inline widget stylesheets -> belong in resources/qss/app.qss.in.
hits=$(grep -rnE '(->|\.)setStyleSheet\(' "${SCOPE[@]}" --include=*.cpp 2>/dev/null \
         | drop_noise)
if [ -n "$hits" ]; then
  echo "FAIL: inline setStyleSheet() in UI code."
  echo "      Move the rule to resources/qss/app.qss.in and target it by objectName,"
  echo "      or add '// theme-exempt: <reason>' if it is genuinely dynamic data."
  echo "$hits" | sed 's/^/  /'
  echo
  status=1
fi

# 2) Hardcoded numeric QColor literals in paint code -> use a token.
hits=$(grep -rnE 'QColor\((0x)?[0-9]' "${SCOPE[@]}" --include=*.cpp 2>/dev/null \
         | drop_noise)
if [ -n "$hits" ]; then
  echo "FAIL: hardcoded QColor(...) literal in rendering code."
  echo "      Add a token to resources/themes/dark.json + src/theme/tokens.h and read it"
  echo "      via ntColor()/ThemeManager::instance().theme().color(), or mark // theme-exempt."
  echo "$hits" | sed 's/^/  /'
  echo
  status=1
fi

if [ "$status" -eq 0 ]; then
  echo "theme hygiene: OK — no un-exempted inline stylesheets or hardcoded colors in UI code."
fi
exit $status
