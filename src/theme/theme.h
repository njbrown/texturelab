#pragma once

#include <QColor>
#include <QFont>
#include <QHash>
#include <QJsonObject>
#include <QString>

// A resolved, immutable snapshot of one theme. Produced by ThemeManager from a
// theme JSON file (all "@ref" indirection already flattened to concrete values).
//
// Consumers:
//   - QssBuilder reads the flat string map (qssVars) for {{token}} substitution.
//   - Custom paint code reads typed values via color()/radius()/space()/font().
//   - ThemeManager reads paletteSpec to build a QPalette.
class Theme
{
public:
    Theme() = default;

    // Typed lookups for paint-time code. A missing color returns a loud magenta
    // (so a mistyped token is obvious on screen rather than silently black).
    QColor color(const QString& key) const;
    bool hasColor(const QString& key) const { return m_colors.contains(key); }
    int radius(const QString& key) const { return m_ints.value("radius." + key, 0); }
    int space(const QString& key) const { return m_ints.value("space." + key, 0); }
    int motion(const QString& key) const { return m_ints.value("motion." + key, 0); }
    QFont font(const QString& key) const { return m_fonts.value(key); }

    const QString& name() const { return m_name; }

    // Flat name->string map used for QSS {{token}} substitution. Colors are
    // "#rrggbb", numbers stringified, font sub-fields as "font.ui.family" etc.
    const QHash<QString, QString>& qssVars() const { return m_qssVars; }

    // role -> resolved QColor, used to build the QPalette.
    const QHash<QString, QColor>& paletteColors() const { return m_paletteColors; }

private:
    friend class ThemeManager;

    QString m_name;
    QHash<QString, QColor> m_colors;       // "bg.window" -> QColor
    QHash<QString, int> m_ints;            // "radius.sm" / "space.md" / "motion.fast"
    QHash<QString, QFont> m_fonts;         // "ui" / "mono" / "title"
    QHash<QString, QString> m_qssVars;     // flat map for template substitution
    QHash<QString, QColor> m_paletteColors; // "window" / "disabled.text" -> QColor
};
