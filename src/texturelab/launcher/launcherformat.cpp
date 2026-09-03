#include "launcherformat.h"

#include "texturelistmodel.h"
#include "thememanager.h"
#include "tokens.h"

#include <QCoreApplication>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QDateTime>
#include <QFontMetrics>
#include <QRect>
#include <QStringList>

namespace launcherfmt {

QString relativeTime(qint64 whenMs)
{
    if (whenMs <= 0)
        return QStringLiteral("—");

    const QDateTime when = QDateTime::fromMSecsSinceEpoch(whenMs);
    const qint64 secs = when.secsTo(QDateTime::currentDateTime());

    if (secs < 60)
        return QCoreApplication::translate("launcher", "just now");
    if (secs < 3600)
        return QStringLiteral("%1m").arg(secs / 60);
    if (secs < 86400)
        return QStringLiteral("%1h").arg(secs / 3600);
    if (secs < 172800)
        return QCoreApplication::translate("launcher", "yesterday");
    if (secs < 604800)
        return QStringLiteral("%1d").arg(secs / 86400);
    if (secs < 2592000)
        return QStringLiteral("%1w").arg(secs / 604800);
    if (secs < 31536000)
        return QStringLiteral("%1mo").arg(secs / 2592000);
    return QStringLiteral("%1y").arg(secs / 31536000);
}

QString resolutionLabel(int width, int height)
{
    if (width <= 0 || height <= 0)
        return QString();

    auto shorten = [](int value) -> QString {
        if (value >= 1024 && value % 1024 == 0)
            return QStringLiteral("%1K").arg(value / 1024);
        return QString::number(value);
    };

    if (width == height)
        return shorten(width);
    return QStringLiteral("%1×%2").arg(shorten(width), shorten(height));
}

QString tooltipFor(const QModelIndex& index, const QString& currentVersionLabel)
{
    if (!index.isValid())
        return QString();

    QStringList lines;

    // Always the path: with no folder tree in the window, this is the only
    // place a texture's location is visible.
    lines << index.data(TextureListModel::PathRole).toString();

    if (index.data(TextureListModel::MissingRole).toBool()) {
        lines << QCoreApplication::translate("launcher",
                                             "Not found on disk — open it to locate the file.");
    }

    if (index.data(TextureListModel::NeedsMigrationRole).toBool()) {
        const QString from = index.data(TextureListModel::LibVersionRole).toString();
        lines << QCoreApplication::translate(
                     "launcher", "Created with library %1; opening offers an upgrade to %2.")
                     .arg(from, currentVersionLabel);
    }

    const int nodes = index.data(TextureListModel::NodeCountRole).toInt();
    if (nodes > 0)
        lines << QCoreApplication::translate("launcher", "%n node(s)", nullptr, nodes);

    return lines.join(QLatin1Char('\n'));
}

void paintStar(QPainter* painter, const QRectF& box, const QColor& color)
{
    QPainterPath path;
    const QPointF center = box.center();
    const double outer = box.width() / 2.0;
    const double inner = outer * 0.45;

    for (int i = 0; i < 10; ++i) {
        const double radius = (i % 2 == 0) ? outer : inner;
        const double angle = -M_PI / 2.0 + i * M_PI / 5.0;
        const QPointF point(center.x() + radius * std::cos(angle),
                            center.y() + radius * std::sin(angle));

        // moveTo for the first point, not lineTo: a fresh QPainterPath starts at
        // the origin, so lineTo here drags a stroke across the whole widget.
        if (i == 0)
            path.moveTo(point);
        else
            path.lineTo(point);
    }
    path.closeSubpath();

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawPath(path);
}

namespace {

QFont versionBadgeFont()
{
    QFont font = ThemeManager::instance().theme().font("ui");

    // Step down in whichever unit the theme actually set — it builds fonts with
    // setPixelSize, which leaves pointSize() at -1.
    if (font.pixelSize() > 0)
        font.setPixelSize(qMax(9, font.pixelSize() - 2));
    else
        font.setPointSize(qMax(7, font.pointSize() - 2));

    return font;
}

} // namespace

QSize versionBadgeSize(const QString& version)
{
    if (version.isEmpty())
        return QSize();

    const QFontMetrics metrics(versionBadgeFont());
    return QSize(metrics.horizontalAdvance(version) + 8, metrics.height() + 2);
}

void paintVersionBadge(QPainter* painter, const QRect& rect, const QString& version)
{
    if (version.isEmpty() || rect.isEmpty())
        return;

    const Theme& theme = ThemeManager::instance().theme();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Translucent dark plate rather than a solid one: it sits on top of the
    // thumbnail, and a fully opaque chip punches a hole in the material.
    QColor plate = theme.color(Tokens::BgElevated);
    plate.setAlpha(185);

    painter->setPen(Qt::NoPen);
    painter->setBrush(plate);

    const int radius = qMax(2, theme.radius("sm"));
    painter->drawRoundedRect(rect, radius, radius);

    painter->setFont(versionBadgeFont());
    painter->setPen(theme.color(Tokens::LauncherBadge));
    painter->drawText(rect, Qt::AlignCenter | Qt::TextSingleLine, version);

    painter->restore();
}

} // namespace launcherfmt
