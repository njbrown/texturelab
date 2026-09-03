#pragma once

#include <QStyledItemDelegate>
#include <QString>

// Paints one texture card: square thumbnail, name, and a "modified · resolution"
// line, with channel pips along the bottom of the thumbnail.
//
// All colors come from the theme at paint time, so --dev-theme hot-reload works
// without a rebuild. No QColor literals here — see scripts/check-theme-hygiene.sh.
class TextureCardDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit TextureCardDelegate(QObject* parent = nullptr);

    // Card width in pixels; the thumbnail is square, so height follows. Set by
    // LauncherWindow::relayoutGrid(), which stretches the slider's target width
    // to whatever divides the viewport evenly — not by the slider directly.
    void setCardWidth(int width);
    int cardWidth() const { return cardW; }

    // The card height a given width implies. The grid needs it to state its
    // cell size, which it has to do before handing the width over here.
    int heightForWidth(int width) const { return width + textBlockHeight(); }

    static constexpr int MinCardWidth = 96;
    static constexpr int MaxCardWidth = 256;

    // Shown in the migration tooltip as the version a file would be upgraded
    // to. Injected so the delegate keeps no dependency on the node library.
    void setCurrentVersionLabel(const QString& label) { currentVersionLabel = label; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    // Builds the tooltip from roles rather than letting the model return a
    // display string — the model stays formatting-free so a second view can
    // present the same data differently.
    bool helpEvent(QHelpEvent* event, QAbstractItemView* view,
                   const QStyleOptionViewItem& option, const QModelIndex& index) override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int textBlockHeight() const;

    int cardW = 160;
    QString currentVersionLabel;
};
