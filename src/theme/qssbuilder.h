#pragma once

#include <QString>

class Theme;

// Turns an authored QSS template (with {{token}} placeholders) into a final
// stylesheet string by substituting values from the active theme.
namespace QssBuilder {

// Substitute every {{token}} in `templateText` with theme.qssVars()[token].
// Unknown tokens are left as-is and a warning is logged, so a typo is visible
// in the rendered CSS rather than silently blanking a rule.
QString build(const QString& templateText, const Theme& theme);

} // namespace QssBuilder
