#include "nodesearchpopup.h"
#include "../utils.h"
#include "./libraries/library.h"

#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

NodeSearchPopup::NodeSearchPopup(QWidget* parent) : QFrame(parent)
{
    library = nullptr;

    // Setup frame styling for a floating popup (see #NodeSearchPopup in app.qss.in)
    setObjectName("NodeSearchPopup");
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

    // Set fixed size for the popup
    setFixedSize(300, 400);

    // Setup layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // Search input
    searchInput = new QLineEdit(this);
    searchInput->setPlaceholderText("Search nodes...");
    searchInput->installEventFilter(this);
    connect(searchInput, &QLineEdit::textChanged, this,
            &NodeSearchPopup::filterList);
    layout->addWidget(searchInput);

    // Item list
    itemList = new QListWidget(this);
    itemList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(itemList, &QListWidget::itemClicked,
            [this](QListWidgetItem* item) {
                if (item) {
                    auto type = (PopupItemType)item->data(Qt::UserRole).toInt();
                    emit itemSelected(item->text(), type, showPosition);
                    hide();
                }
            });
    layout->addWidget(itemList);

    setLayout(layout);
}

void NodeSearchPopup::setLibrary(Library* lib)
{
    library = lib;
    itemList->clear();

    // Fixed entries: Frame and Comment always appear at the top
    auto frameItem = new QListWidgetItem(QIcon(":nodes/frame.png"), "Frame");
    frameItem->setData(Qt::UserRole, (int)PopupItemType::Frame);
    itemList->addItem(frameItem);

    auto commentItem = new QListWidgetItem(QIcon(":nodes/comment.png"), "Comment");
    commentItem->setData(Qt::UserRole, (int)PopupItemType::Comment);
    itemList->addItem(commentItem);

    if (lib) {
        for (auto& libraryItem : lib->items) {
            QListWidgetItem* item = new QListWidgetItem;
            item->setText(libraryItem.name);
            item->setIcon(libraryItem.icon);
            item->setData(Qt::UserRole, (int)PopupItemType::Node);
            itemList->addItem(item);
        }
    }

    if (itemList->count() > 0) {
        itemList->setCurrentRow(0);
    }
}

void NodeSearchPopup::show(const QPoint& pos)
{
    showPosition = pos;

    // Clear search and reset filter
    searchInput->clear();
    filterList("");

    // Select first item
    if (itemList->count() > 0) {
        itemList->setCurrentRow(0);
    }

    // Position the popup at the cursor
    move(pos);

    // Show the popup
    QFrame::show();

    // Focus the search input
    searchInput->setFocus();
}

void NodeSearchPopup::hide()
{
    QFrame::hide();
    searchInput->clear();
}

void NodeSearchPopup::filterList(const QString& text)
{
    QString searchText = text.toLower();

    int visibleCount = 0;
    for (int i = 0; i < itemList->count(); i++) {
        QListWidgetItem* item = itemList->item(i);
        QString itemName = item->text().toLower();

        bool matches = searchText.isEmpty() || itemName.contains(searchText);
        item->setHidden(!matches);

        if (matches && visibleCount == 0) {
            // Select the first visible item
            itemList->setCurrentItem(item);
        }

        if (matches)
            visibleCount++;
    }
}

void NodeSearchPopup::selectItem(int index)
{
    if (index >= 0 && index < itemList->count()) {
        itemList->setCurrentRow(index);
    }
}

QString NodeSearchPopup::getSelectedItemName() const
{
    auto currentItem = itemList->currentItem();
    if (currentItem && !currentItem->isHidden()) {
        return currentItem->text();
    }
    return QString();
}

PopupItemType NodeSearchPopup::getSelectedItemType() const
{
    auto currentItem = itemList->currentItem();
    if (currentItem && !currentItem->isHidden()) {
        return (PopupItemType)currentItem->data(Qt::UserRole).toInt();
    }
    return PopupItemType::Node;
}

bool NodeSearchPopup::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == searchInput && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // Handle arrow keys to navigate the list
        if (keyEvent->key() == Qt::Key_Up) {
            // Navigate up in the list (only through visible items)
            int currentRow = itemList->currentRow();
            for (int i = currentRow - 1; i >= 0; i--) {
                if (!itemList->item(i)->isHidden()) {
                    itemList->setCurrentRow(i);
                    break;
                }
            }
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Down) {
            // Navigate down in the list (only through visible items)
            int currentRow = itemList->currentRow();
            for (int i = currentRow + 1; i < itemList->count(); i++) {
                if (!itemList->item(i)->isHidden()) {
                    itemList->setCurrentRow(i);
                    break;
                }
            }
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Return ||
                 keyEvent->key() == Qt::Key_Enter) {
            // Add the selected item
            QString itemName = getSelectedItemName();
            if (!itemName.isEmpty()) {
                emit itemSelected(itemName, getSelectedItemType(), showPosition);
                hide();
            }
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Escape) {
            // Close the popup
            hide();
            return true;
        }
    }

    return QFrame::eventFilter(obj, event);
}

void NodeSearchPopup::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        // Add the selected item
        QString itemName = getSelectedItemName();
        if (!itemName.isEmpty()) {
            emit itemSelected(itemName, getSelectedItemType(), showPosition);
            hide();
        }
        event->accept();
        break;
    }
    case Qt::Key_Escape:
        // Close the popup
        hide();
        event->accept();
        break;
    case Qt::Key_Up: {
        // Navigate up in the list (only through visible items)
        int currentRow = itemList->currentRow();
        for (int i = currentRow - 1; i >= 0; i--) {
            if (!itemList->item(i)->isHidden()) {
                itemList->setCurrentRow(i);
                break;
            }
        }
        event->accept();
        break;
    }
    case Qt::Key_Down: {
        // Navigate down in the list (only through visible items)
        int currentRow = itemList->currentRow();
        for (int i = currentRow + 1; i < itemList->count(); i++) {
            if (!itemList->item(i)->isHidden()) {
                itemList->setCurrentRow(i);
                break;
            }
        }
        event->accept();
        break;
    }
    default:
        QFrame::keyPressEvent(event);
        break;
    }
}
