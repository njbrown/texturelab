#pragma once

#include <QFrame>
#include <QListWidget>
#include <QMimeData>

class Library;
class LibraryListWidget;
class QLineEdit;

// https://stackoverflow.com/questions/37331270/how-to-create-grid-style-qlistwidget
class LibraryWidget : public QWidget {
    // Q_OBJECT
public:
    LibraryWidget();
    void setLibrary(Library* lib);

    LibraryListWidget* listWidget;
    QLineEdit* searchBar;
};

class LibraryItemMimeData : public QMimeData {
    Q_OBJECT

public:
    QString libraryItemName;

    virtual bool hasFormat(const QString& format) const override;
};

class LibraryListWidget : public QListWidget {
public:
    LibraryListWidget();

protected:
    // https://stackoverflow.com/questions/4295838/how-to-implement-drag-in-a-qlistwidget-that-contains-files
    // QStringList mimeTypes() const
    // {
    //     QStringList qstrList;
    //     qstrList.append("texturelab/library-item");
    //     return qstrList;
    // }

    QMimeData* mimeData(const QList<QListWidgetItem*>& items) const;
};