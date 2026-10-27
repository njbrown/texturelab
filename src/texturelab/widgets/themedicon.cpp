#include "themedicon.h"
#include "thememanager.h"
#include "tokens.h"

#include <QPainter>
#include <QPixmap>

static QPixmap tintPixmap(const QPixmap& src, const QColor& color)
{
    QPixmap out(src.size());
    out.setDevicePixelRatio(src.devicePixelRatio());
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.drawPixmap(0, 0, src);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(out.rect(), color);
    p.end();
    return out;
}

QIcon themedToolIcon(const QString& svgPath)
{
    const Theme& t = ThemeManager::instance().theme();
    const QPixmap base = QIcon(svgPath).pixmap(QSize(32, 32)); // white source SVG
    QIcon icon;
    icon.addPixmap(tintPixmap(base, t.color(Tokens::TextPrimary)), QIcon::Normal);
    icon.addPixmap(tintPixmap(base, t.color(Tokens::TextDisabled)), QIcon::Disabled);
    return icon;
}
