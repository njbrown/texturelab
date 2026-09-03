#include "librarywidget.h"
#include "../utils.h"
#include "./libraries/library.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMimeData>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStyle>
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
    this->setObjectName("LibraryPanel"); // QSS scoping (app.qss.in)
    this->setMinimumWidth(100);
    this->setLayout(new QVBoxLayout());

    // library version indicator + upgrade button
    auto versionRow = new QWidget(this);
    auto versionLayout = new QHBoxLayout(versionRow);
    versionLayout->setContentsMargins(0, 0, 0, 0);

    versionLabel = new QLabel(versionRow);
    versionLabel->setObjectName("LibraryVersionLabel"); // styled in app.qss.in
    versionLayout->addWidget(versionLabel);

    versionLayout->addStretch();

    upgradeButton = new QPushButton("Upgrade", versionRow);
    upgradeButton->setProperty("variant", "primary"); // draw attention to the action
    upgradeButton->setVisible(false);
    connect(upgradeButton, &QPushButton::clicked,
            this, &LibraryWidget::upgradeRequested);
    versionLayout->addWidget(upgradeButton);

    this->layout()->addWidget(versionRow);

    // search box
    searchBar = new QLineEdit(this);
    searchBar->setObjectName("LibrarySearch");
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

void LibraryWidget::setLibraryVersion(const QString& version, bool isCurrent)
{
    versionLabel->setText(isCurrent
                              ? QString("Library: %1").arg(version)
                              : QString("Library: %1 (outdated)").arg(version));

    // Drive the color from a dynamic property so the "outdated" tint lives in
    // app.qss.in (uses the theme's warn token) rather than a hardcoded hex.
    versionLabel->setProperty("outdated", !isCurrent);
    versionLabel->style()->unpolish(versionLabel);
    versionLabel->style()->polish(versionLabel);

    upgradeButton->setVisible(!isCurrent);
}

void LibraryWidget::addSpecialItem(const QString& name,
                                    const QString& iconPath, PopupItemType type)
{
    QListWidgetItem* item = new QListWidgetItem;
    item->setData(Qt::DisplayRole, name);
    item->setData((int)Roles::ItemType, "LibraryItem");
    item->setData((int)Roles::LibraryItemName, name);
    item->setData(Qt::UserRole, (int)type);
    item->setSizeHint(QSize(90, 90));
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setIcon(QIcon(iconPath));
    this->listWidget->addItem(item);
}

void LibraryWidget::setLibrary(Library* lib)
{
    this->listWidget->clear();

    addSpecialItem("Frame", ":nodes/frame.png", PopupItemType::Frame);
    addSpecialItem("Comment", ":nodes/comment.png", PopupItemType::Comment);

    if (!lib)
        return;

    for (auto& libraryItem : lib->items) {
        QListWidgetItem* item = new QListWidgetItem;
        item->setData(Qt::DisplayRole, libraryItem.name);
        item->setData((int)Roles::ItemType, "LibraryItem");
        item->setData((int)Roles::LibraryItemName, libraryItem.name);
        item->setData(Qt::UserRole, (int)PopupItemType::Node);
        item->setSizeHint(QSize(90, 90));
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
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

    setObjectName("LibraryList"); // item styling in app.qss.in
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
    auto itemType = (PopupItemType)items[0]->data(Qt::UserRole).toInt();
    qDebug() << "Mime Data Dragging: " << itemName;

    auto mimeData = new LibraryItemMimeData();
    mimeData->libraryItemName = itemName;
    mimeData->itemType = itemType;

    return mimeData;
}