#include "theme.h"

QColor Theme::color(const QString& key) const
{
    auto it = m_colors.constFind(key);
    if (it == m_colors.constEnd()) {
        qWarning("Theme: unknown color token '%s'", qPrintable(key));
        return QColor(255, 0, 255); // loud magenta = obvious mistake
    }
    return it.value();
}
