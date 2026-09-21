#pragma once

#include <QModelIndex>
#include <QString>

class QColor;
class QPainter;
class QRect;
class QRectF;
class QSize;

// Presentation helpers shared by the card and row delegates.
//
// This is where the launcher's formatting lives, deliberately outside the
// model: TextureListModel exposes raw values so that two views can present the
// same row differently, and both views agreeing on "2h" and "4K" is a matter of
// sharing this file, not of pushing strings back into the model.
namespace launcherfmt {

// "just now", "2h", "yesterday", "3d", "1mo" — short enough to sit under a 96px
// card without eliding. Returns an em dash for "never".
QString relativeTime(qint64 whenMs);

// 4096 -> "4K", 1024 -> "1K", 512 -> "512", non-square -> "2K×1K". Empty when
// the resolution isn't known, which is the case for rows seeded from the old
// recent-files list and not yet opened.
QString resolutionLabel(int width, int height);

// The full path, plus whatever explains an otherwise cryptic marker on the
// card: the missing state, the migration badge, the node count.
QString tooltipFor(const QModelIndex& index, const QString& currentVersionLabel);

// A five-pointed star filling `box`. Shared so "starred" reads the same in the
// grid and the list — as a shape, not just a color. The migration badge is a
// dot in the same warn color, and two identical dots side by side in a row say
// nothing.
void paintStar(QPainter* painter, const QRectF& box, const QColor& color);

// The library-version chip ("v1") shown on textures written by an older node
// library. A bare colored dot said only "something is unusual"; the version
// number says which one, which is the thing worth knowing before opening.
// Size first so the caller can place it, then paint into that rect.
QSize versionBadgeSize(const QString& version);
void paintVersionBadge(QPainter* painter, const QRect& rect, const QString& version);

} // namespace launcherfmt
