#pragma once

#include <QString>
#include <QStyledItemDelegate>

// Paints one texture as a table-like row: small thumbnail, name, then aligned
// columns for modified / opened / resolution / nodes.
//
// Shares the model with TextureCardDelegate and reads exactly the same roles —
// which is the whole reason the model exposes typed values and formats nothing.
// Past a few hundred textures a list beats a grid for finding a known name, and
// Resolve ships both for the same reason.
class TextureRowDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit TextureRowDelegate(QObject* parent = nullptr);

    void setCurrentVersionLabel(const QString& label) { currentVersionLabel = label; }

    static constexpr int ThumbSize = 40;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    bool helpEvent(QHelpEvent* event, QAbstractItemView* view,
                   const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    QString currentVersionLabel;
};
