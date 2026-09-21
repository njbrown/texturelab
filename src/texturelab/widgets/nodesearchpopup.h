#pragma once

#include <QFrame>
#include <QListWidget>

class Library;
class QLineEdit;

enum class PopupItemType { Node, Frame, Comment };

class NodeSearchPopup : public QFrame {
    Q_OBJECT

public:
    NodeSearchPopup(QWidget* parent = nullptr);

    void setLibrary(Library* lib);
    void show(const QPoint& pos);
    void hide();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void filterList(const QString& text);
    void selectItem(int index);
    QString getSelectedItemName() const;
    PopupItemType getSelectedItemType() const;

    Library* library;
    QLineEdit* searchInput;
    QListWidget* itemList;
    QPoint showPosition;

signals:
    void itemSelected(const QString& itemName, PopupItemType type,
                      const QPoint& position);
};
