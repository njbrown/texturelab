#include "librarywidget.h"
#include "library.h"

#include <QListWidget>
#include <QFont>
#include <QScrollBar>
#include <QListWidgetItem>
#include <QLayout>
#include <QVBoxLayout>
#include <QLineEdit>

LibraryWidget::LibraryWidget() : QWidget()
{
    this->setMinimumWidth(100);
    this->setLayout(new QVBoxLayout());

    // search box
    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("search");
    searchBar->setAlignment(Qt::AlignLeft);
    connect(searchBar, &QLineEdit::textChanged, [=](QString text)
            { qDebug() << "search changed"; });

    this->layout()->addWidget(searchBar);

    // list widget
    listWidget = new LibraryListWidget();
    listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->layout()->addWidget(listWidget);

    this->setLibrary(nullptr);
}

void LibraryWidget::setLibrary(Library *lib)
{
    QSize currentSize = QSize(90, 90);
    // for (auto &libraryItem : lib->items)
    for (int i = 0; i < 10; i++)
    {
        QListWidgetItem *item = new QListWidgetItem;
        item->setData(Qt::DisplayRole, "Node");
        // item->setData(Qt::DisplayRole, libraryItem.name);

        item->setSizeHint(currentSize);
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setIcon(QIcon(":nodes/bevel.png"));

        this->listWidget->addItem(item);
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

    setDragDropMode(QAbstractItemView::DragOnly);
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
        "QListView::item{ border-radius: 2px; border: 0px solid rgba(0,0,0,1); margin-left: 6px;  }"
        "QListView::item:hover{border: 1px solid rgba(50,150,250,1); }");
}