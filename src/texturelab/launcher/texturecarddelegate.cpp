#include "texturecarddelegate.h"

#include "launcherformat.h"
#include "texturelistmodel.h"
#include "texturerecord.h"
#include "thememanager.h"
#include "tokens.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHelpEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

namespace {

QColor tc(const char* token)
{
    return ThemeManager::instance().theme().color(token);
}

QColor tc(const char* token, int alpha)
{
    QColor color = ThemeManager::instance().theme().color(token);
    color.setAlpha(alpha);
    return color;
}

int themeRadius()
{
    const int r = ThemeManager::instance().theme().radius("sm");
    return r > 0 ? r : 3;
}

int themeSpace(const char* key, int fallback)
{
    const int s = ThemeManager::instance().theme().space(key);
    return s > 0 ? s : fallback;
}

// The channel pips, in a fixed order so a card's silhouette is recognizable at
// a glance rather than shuffling with whatever the graph happens to define.
const int kPipOrder[] = {
    catalog::ChannelAlbedo, catalog::ChannelNormal,   catalog::ChannelRoughness,
    catalog::ChannelHeight, catalog::ChannelMetalness, catalog::ChannelAO,
};
constexpr int kPipCount = int(sizeof(kPipOrder) / sizeof(kPipOrder[0]));

} // namespace

TextureCardDelegate::TextureCardDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void TextureCardDelegate::setCardWidth(int width)
{
    const int clamped = qBound(MinCardWidth, width, MaxCardWidth);
    if (clamped == cardW)
        return;

    cardW = clamped;

    // Tells the view its cached geometry is stale. An invalid index means "all
    // of them", which is exactly the case when the card size changes.
    emit sizeHintChanged(QModelIndex());
}

int TextureCardDelegate::textBlockHeight() const
{
    // Name, a gap, then "modified · resolution". The gap is load-bearing: with
    // the two lines flush, a name ending in an underscore paints its glyph onto
    // the metadata line below.
    const QFontMetrics metrics(ThemeManager::instance().theme().font("ui"));
    return metrics.lineSpacing() * 2 + themeSpace("sm", 4) * 3;
}

QSize TextureCardDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    return QSize(cardW, cardW + textBlockHeight());
}

void TextureCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
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
    const int radius = themeRadius();

    QRect card = option.rect.adjusted(pad, pad, -pad, -pad);
    const int thumbSide = card.width();
    QRect thumb(card.left(), card.top(), thumbSide, thumbSide);

    // Card body. Selection is a border, never a fill — the material renders are
    // supposed to be the only saturated thing on screen.
    painter->setPen(Qt::NoPen);
    painter->setBrush(hovered ? tc(Tokens::LauncherCardHover) : tc(Tokens::LauncherCard));
    painter->drawRoundedRect(card, radius, radius);

    // Thumbnail well. Drawn even when there's an image, so the rounded corners
    // and the "no preview yet" state share one shape.
    painter->setBrush(tc(Tokens::LauncherThumbBg));
    painter->drawRoundedRect(thumb, radius, radius);

    const QPixmap preview = index.data(TextureListModel::ThumbnailRole).value<QPixmap>();
    if (!preview.isNull()) {
        painter->save();

        // Clip to the well so the capture inherits its rounded corners instead
        // of painting square over them.
        QPainterPath clip;
        clip.addRoundedRect(thumb, radius, radius);
        painter->setClipPath(clip);

        // The stored image is square and so is the well, but the card can be
        // resized to anything — scale to cover and center rather than letting a
        // rounding difference letterbox it.
        const QPixmap scaled = preview.scaled(thumb.size(), Qt::KeepAspectRatioByExpanding,
                                              Qt::SmoothTransformation);
        painter->drawPixmap(thumb.center() - QPoint(scaled.width() / 2, scaled.height() / 2),
                            scaled);
        painter->restore();
    }

    if (missing)
        painter->setOpacity(0.4);

    // Channel pips along the bottom of the thumbnail.
    const int channels = index.data(TextureListModel::ChannelsRole).toInt();
    if (channels != catalog::ChannelNone) {
        const int pipSize = qMax(3, thumbSide / 32);
        const int gap = pipSize;
        const int totalWidth = kPipCount * pipSize + (kPipCount - 1) * gap;
        int x = thumb.left() + (thumb.width() - totalWidth) / 2;
        const int y = thumb.bottom() - pipSize - pad;

        painter->setPen(Qt::NoPen);
        for (int i = 0; i < kPipCount; ++i) {
            const bool on = (channels & kPipOrder[i]) != 0;
            painter->setBrush(on ? tc(Tokens::LauncherPipOn) : tc(Tokens::LauncherPipOff, 120));
            painter->drawEllipse(QRect(x, y, pipSize, pipSize));
            x += pipSize + gap;
        }
    }

    painter->setOpacity(1.0);

    // Text block.
    const QFont uiFont = ThemeManager::instance().theme().font("ui");

    // Bounded by the card, not by textBlockHeight(): the latter is the sizeHint
    // budget and overruns the card body by one pad, which clips the descenders
    // of the meta line against the border.
    QRect textRect(card.left() + pad, thumb.bottom() + pad, card.width() - pad * 2,
                   card.bottom() - thumb.bottom() - pad * 2);

    QFont nameFont = uiFont;
    QFontMetrics nameMetrics(nameFont);
    nameFont.setStrikeOut(missing);
    painter->setFont(nameFont);
    painter->setPen(missing ? tc(Tokens::TextDisabled) : tc(Tokens::TextPrimary));

    const QString name = index.data(TextureListModel::NameRole).toString();
    // lineSpacing(), not height(): height() ends flush with the descent, and a
    // name containing an underscore then paints its glyph right on the meta
    // line below, which reads as a stray dash next to the metadata.
    QRect nameRect(textRect.left(), textRect.top(), textRect.width(), nameMetrics.lineSpacing());
    // TextSingleLine: without it drawText() may lay the string out as wrapped
    // rich-ish text in a rect this short, which leaves a clipped fragment of the
    // overflow visible as a stray mark next to the line.
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                      nameMetrics.elidedText(name, Qt::ElideMiddle, nameRect.width()));

    // Secondary line: "2h · 4K", or the missing marker, which matters more than
    // either of them.
    QFont metaFont = uiFont;
    // The theme builds fonts with setPixelSize (thememanager.cpp), so
    // pointSize() returns -1 on them. Subtracting from that silently yields a
    // 6pt font — small enough that the renderer leaves a stray hinting artifact
    // past the end of the string. Step down in whichever unit is actually set.
    if (uiFont.pixelSize() > 0)
        metaFont.setPixelSize(qMax(9, uiFont.pixelSize() - 1));
    else
        metaFont.setPointSize(qMax(7, uiFont.pointSize() - 1));
    painter->setFont(metaFont);
    painter->setPen(tc(Tokens::TextSecondary));

    QString meta;
    if (missing) {
        meta = QStringLiteral("missing");
    }
    else {
        meta = launcherfmt::relativeTime(index.data(TextureListModel::ModifiedRole).toLongLong());
        const QString resolution = launcherfmt::resolutionLabel(
            index.data(TextureListModel::WidthRole).toInt(),
            index.data(TextureListModel::HeightRole).toInt());
        if (!resolution.isEmpty())
            meta += QStringLiteral(" · ") + resolution;
    }

    const QFontMetrics metaMetrics(metaFont);
    QRect metaRect(textRect.left(), nameRect.bottom() + themeSpace("sm", 4) / 2, textRect.width(),
                   metaMetrics.height());
    painter->drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                      metaMetrics.elidedText(meta, Qt::ElideRight, metaRect.width()));


    // Overlays on the thumbnail corners.
    const int markSize = qMax(10, thumbSide / 10);

    if (starred)
        launcherfmt::paintStar(painter, QRectF(thumb.right() - markSize - pad, thumb.top() + pad, markSize,
                                 markSize),
                 tc(Tokens::LauncherStar));

    if (isOpen) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(tc(Tokens::LauncherOpenDot));
        painter->drawEllipse(QRect(thumb.left() + pad, thumb.top() + pad, markSize / 2,
                                   markSize / 2));
    }

    if (needsMigration) {
        // The version itself, not an anonymous marker: "v1" tells you which
        // library wrote this and therefore what the upgrade prompt will offer.
        const QString version = index.data(TextureListModel::LibVersionRole).toString();
        const QSize badge = launcherfmt::versionBadgeSize(version);
        if (!badge.isEmpty()) {
            launcherfmt::paintVersionBadge(
                painter,
                QRect(QPoint(thumb.left() + pad, thumb.bottom() - badge.height() - pad), badge),
                version);
        }
    }

    // Borders last, so nothing paints over them.
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(selected ? tc(Tokens::Accent) : tc(Tokens::LauncherCardBorder), 1));
    painter->drawRoundedRect(card.adjusted(0, 0, -1, -1), radius, radius);

    painter->restore();
}

bool TextureCardDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view,
                                    const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (!event || !view || !index.isValid())
        return QStyledItemDelegate::helpEvent(event, view, option, index);

    QToolTip::showText(event->globalPos(),
                       launcherfmt::tooltipFor(index, currentVersionLabel), view);
    return true;
}
