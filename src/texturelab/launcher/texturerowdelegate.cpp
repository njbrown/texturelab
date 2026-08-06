#include "texturerowdelegate.h"

#include "launcherformat.h"
#include "texturelistmodel.h"
#include "texturerecord.h"
#include "thememanager.h"
#include "tokens.h"

#include <QAbstractItemView>
#include <QHelpEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

namespace {

QColor tc(const char* token)
{
    return ThemeManager::instance().theme().color(token);
}

int themeSpace(const char* key, int fallback)
{
    const int s = ThemeManager::instance().theme().space(key);
    return s > 0 ? s : fallback;
}

// Fixed right-hand columns, widest first so they don't jitter as content
// changes. The name takes whatever is left.
constexpr int kModifiedWidth = 96;
constexpr int kOpenedWidth = 96;
constexpr int kResolutionWidth = 72;
constexpr int kNodesWidth = 64;

} // namespace

TextureRowDelegate::TextureRowDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QSize TextureRowDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    const int pad = themeSpace("sm", 4);
    return QSize(0, ThumbSize + pad * 2);
}

void TextureRowDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                               const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    const bool missing = index.data(TextureListModel::MissingRole).toBool();
    const bool starred = index.data(TextureListModel::StarredRole).toBool();
    const bool isOpen = index.data(TextureListModel::IsOpenRole).toBool();
    const bool needsMigration = index.data(TextureListModel::NeedsMigrationRole).toBool();

    const int pad = themeSpace("sm", 4);
    const int radius = ThemeManager::instance().theme().radius("sm");

    const QRect row = option.rect;

    if (hovered || selected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(selected ? tc(Tokens::LauncherCard) : tc(Tokens::LauncherCardHover));
        painter->drawRect(row);
    }

    // Thumbnail.
    QRect thumb(row.left() + pad * 2, row.top() + pad, ThumbSize, ThumbSize);
    painter->setPen(Qt::NoPen);
    painter->setBrush(tc(Tokens::LauncherThumbBg));
    painter->drawRoundedRect(thumb, radius, radius);

    const QPixmap preview = index.data(TextureListModel::ThumbnailRole).value<QPixmap>();
    if (!preview.isNull()) {
        painter->save();
        QPainterPath clip;
        clip.addRoundedRect(thumb, radius, radius);
        painter->setClipPath(clip);
        const QPixmap scaled = preview.scaled(thumb.size(), Qt::KeepAspectRatioByExpanding,
                                              Qt::SmoothTransformation);
        painter->drawPixmap(thumb.center() - QPoint(scaled.width() / 2, scaled.height() / 2),
                            scaled);
        painter->restore();
    }

    const QFont uiFont = ThemeManager::instance().theme().font("ui");
    const QFontMetrics metrics(uiFont);
    painter->setFont(uiFont);

    // Right-hand columns are laid out from the right edge inwards, so the name
    // column absorbs the window width rather than the numbers drifting.
    int right = row.right() - pad * 2;

    auto column = [&](int width, const QString& text) {
        const QRect cell(right - width, row.top(), width, row.height());
        painter->drawText(cell, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine,
                          metrics.elidedText(text, Qt::ElideRight, width - pad));
        right -= width;
    };

    painter->setPen(tc(Tokens::TextSecondary));

    const int nodes = index.data(TextureListModel::NodeCountRole).toInt();
    column(kNodesWidth, nodes > 0 ? QStringLiteral("%1").arg(nodes) : QString());
    column(kResolutionWidth,
           launcherfmt::resolutionLabel(index.data(TextureListModel::WidthRole).toInt(),
                                        index.data(TextureListModel::HeightRole).toInt()));
    column(kOpenedWidth,
           launcherfmt::relativeTime(index.data(TextureListModel::OpenedRole).toLongLong()));
    column(kModifiedWidth,
           missing ? QObject::tr("missing")
                   : launcherfmt::relativeTime(
                         index.data(TextureListModel::ModifiedRole).toLongLong()));

    // Markers sit between the thumbnail and the name so the name column starts
    // at a predictable place whether or not a row carries any.
    int left = thumb.right() + pad * 2;
    const int markSize = metrics.height() / 2;

    if (isOpen) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(tc(Tokens::LauncherOpenDot));
        painter->drawEllipse(QRect(left, row.center().y() - markSize / 2, markSize, markSize));
        left += markSize + pad;
    }
    if (needsMigration) {
        const QString version = index.data(TextureListModel::LibVersionRole).toString();
        const QSize badge = launcherfmt::versionBadgeSize(version);
        if (!badge.isEmpty()) {
            launcherfmt::paintVersionBadge(
                painter,
                QRect(QPoint(left, row.center().y() - badge.height() / 2), badge), version);
            left += badge.width() + pad;
        }
    }
    if (starred) {
        launcherfmt::paintStar(
            painter,
            QRectF(left, row.center().y() - markSize / 2.0, markSize, markSize),
            tc(Tokens::LauncherStar));
        left += markSize + pad;
    }

    QFont nameFont = uiFont;
    nameFont.setStrikeOut(missing);
    painter->setFont(nameFont);
    painter->setPen(missing ? tc(Tokens::TextDisabled) : tc(Tokens::TextPrimary));

    const QRect nameRect(left, row.top(), qMax(0, right - left - pad), row.height());
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                      QFontMetrics(nameFont).elidedText(
                          index.data(TextureListModel::NameRole).toString(), Qt::ElideMiddle,
                          nameRect.width()));

    // Hairline separator, drawn last and only between rows.
    painter->setPen(tc(Tokens::LauncherCardBorder));
    painter->drawLine(row.left(), row.bottom(), row.right(), row.bottom());

    painter->restore();
}

bool TextureRowDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view,
                                   const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (!event || !view || !index.isValid())
        return QStyledItemDelegate::helpEvent(event, view, option, index);

    QToolTip::showText(event->globalPos(),
                       launcherfmt::tooltipFor(index, currentVersionLabel), view);
    return true;
}
