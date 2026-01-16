#include "librarywidget.h"
#include "../utils.h"
#include "./libraries/library.h"

#include <QFont>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMimeData>
#include <QResizeEvent>
#include <QScrollBar>
#include <QVBoxLayout>

// https://doc.qt.io/qt-6/qmimedata.html
// subclassing QMimeData is cleaner

bool LibraryItemMimeData::hasFormat(const QString& format) const
{
    if (format == LIBRARY_ITEM_MIME_FORMAT)
        return true;

    return false;
}

LibraryWidget::LibraryWidget() : QWidget()
{
    this->setMinimumWidth(100);
    this->setLayout(new QVBoxLayout());

    // search box
    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("search");
    searchBar->setAlignment(Qt::AlignLeft);
    connect(searchBar, &QLineEdit::textChanged,
            this, &LibraryWidget::filterList);

    this->layout()->addWidget(searchBar);

    // list widget
    listWidget = new LibraryListWidget();
    listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->layout()->addWidget(listWidget);

    this->setLibrary(nullptr);
}

void LibraryWidget::setLibrary(Library* lib)
{
    QSize currentSize = QSize(90, 90);
    this->listWidget->clear();

    if (!lib)
        return;

    for (auto& libraryItem : lib->items) {
        // for (int i = 0; i < 10; i++) {
        QListWidgetItem* item = new QListWidgetItem;
        item->setData(Qt::DisplayRole, libraryItem.name);
        item->setData((int)Roles::ItemType, "LibraryItem");
        item->setData((int)Roles::LibraryItemName, libraryItem.name);
        // item->setData(Qt::DisplayRole, libraryItem.name);

        item->setSizeHint(currentSize);
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        // item->setIcon(QIcon(":nodes/bevel.png"));
        item->setIcon(libraryItem.icon);

        this->listWidget->addItem(item);
    }
}

void LibraryWidget::filterList(const QString& text)
{
    QString searchText = text.toLower();

    for (int i = 0; i < listWidget->count(); i++) {
        QListWidgetItem* item = listWidget->item(i);
        QString itemName = item->data(Qt::DisplayRole).toString().toLower();

        // Show item if search text is empty or item name contains search text
        bool matches = searchText.isEmpty() || itemName.contains(searchText);
        item->setHidden(!matches);
    }
}

LibraryListWidget::LibraryListWidget() : QListWidget()
{
    setAlternatingRowColors(false);
    setSpacing(0);
    setViewMode(QListWidget::IconMode);
    setIconSize(QSize(70, 70));
    setMouseTracking(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    setDragDropMode(QAbstractItemView::DragDrop);
    setDragEnabled(true);

    setResizeMode(QListWidget::Adjust);
    setDefaultDropAction(Qt::CopyAction);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setWordWrap(true);
    setGridSize(QSize(90, 90));

    setContentsMargins(0, 0, 0, 0);

    // setDragEnabled(true);
    // viewport()->setAcceptDrops(true);
    // setAcceptDrops(true);
    setDropIndicatorShown(true);

    setStyleSheet(
        "QListView::item{ border-radius: 2px; border: 0px solid rgba(0,0,0,1); "
        "margin-left: 6px;  }"
        "QListView::item:hover{border: 1px solid rgba(50,150,250,1); }");
}

void LibraryListWidget::resizeEvent(QResizeEvent* event)
{
    QListWidget::resizeEvent(event);
    updateGridSize();
}

void LibraryListWidget::updateGridSize()
{
    int itemSize = 90;
    int availableWidth = viewport()->width() - verticalScrollBar()->width();
    int itemsPerRow = qMax(1, availableWidth / itemSize);
    int adjustedItemWidth = availableWidth / itemsPerRow;

    setGridSize(QSize(adjustedItemWidth, itemSize));
}

QMimeData*
LibraryListWidget::mimeData(const QList<QListWidgetItem*>& items) const
{
    // QMimeData* data = new QMimeData();
    // // set text for item
    // data->setText(items[0]->data(Qt::DisplayRole).toString());
    // data->setData("ITEM_TYPE", "LIBRARY_ITEM");

    auto itemName = items[0]->data((int)Roles::LibraryItemName).toString();
    // data->setData("LIBRARY_ITEM_NAME", itemName);
    qDebug() << "Mime Data Dragging: " << itemName;

    auto mimeData = new LibraryItemMimeData();
    mimeData->libraryItemName = itemName;

    return mimeData;
}