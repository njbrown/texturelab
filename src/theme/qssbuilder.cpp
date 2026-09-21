#include "qssbuilder.h"

#include "theme.h"

#include <QRegularExpression>

QString QssBuilder::build(const QString& templateText, const Theme& theme)
{
    const QHash<QString, QString>& vars = theme.qssVars();

    // Matches {{ token.name }} with optional surrounding whitespace.
    static const QRegularExpression re(QStringLiteral("\\{\\{\\s*([^}\\s]+)\\s*\\}\\}"));

    QString out;
    out.reserve(templateText.size());

    qsizetype last = 0;
    auto it = re.globalMatch(templateText);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        out += templateText.mid(last, m.capturedStart() - last);

        const QString key = m.captured(1);
        auto found = vars.constFind(key);
        if (found != vars.constEnd()) {
            out += found.value();
        }
        else {
            qWarning("QssBuilder: unknown token '{{%s}}' left unsubstituted",
                     qPrintable(key));
            out += m.captured(0); // leave placeholder so the miss is visible
        }
        last = m.capturedEnd();
    }
    out += templateText.mid(last);
    return out;
}
