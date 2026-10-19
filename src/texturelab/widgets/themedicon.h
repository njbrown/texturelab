#pragma once

#include <QIcon>
#include <QString>

// A QIcon from a white source SVG, with Normal/Disabled variants tinted to the
// theme's text colors, so the icon always matches the button label's color in
// each state (Qt's auto-generated disabled fade doesn't match text.disabled
// exactly).
QIcon themedToolIcon(const QString& svgPath);
