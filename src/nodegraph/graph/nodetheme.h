#pragma once

// Convenience accessors so the node-graph paint code (surface C: QSS can't reach
// QGraphicsItem painting) can pull colors from the shared theme with one call.
// Colors are read at paint time, so they follow theme changes / --dev-theme
// hot-reload as soon as the view repaints (see NodeGraph's themeChanged hookup).

#include "thememanager.h"
#include "tokens.h"

#include <QColor>

inline QColor ntColor(const char* token)
{
    return ThemeManager::instance().theme().color(token);
}

// Same, with an explicit alpha for translucent overlays (socket labels, etc.).
inline QColor ntColor(const char* token, int alpha)
{
    QColor c = ThemeManager::instance().theme().color(token);
    c.setAlpha(alpha);
    return c;
}
