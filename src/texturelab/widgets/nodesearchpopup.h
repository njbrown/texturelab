#pragma once

#include <QFrame>
#include <QListWidget>

class Library;
class QLineEdit;

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

    Library* library;
    QLineEdit* searchInput;
    QListWidget* itemList;
    QPoint showPosition;

signals:
    void itemSelected(const QString& itemName, const QPoint& position);
};
