#pragma once

#include <QFrame>
#include <QListWidget>

class Library;
class LibraryListWidget;
class QLineEdit;

// https://stackoverflow.com/questions/37331270/how-to-create-grid-style-qlistwidget
class LibraryWidget : public QWidget
{
    // Q_OBJECT
public:
    LibraryWidget();
    void setLibrary(Library *lib);

    LibraryListWidget *listWidget;
    QLineEdit *searchBar;
};

class LibraryListWidget : public QListWidget
{
public:
    LibraryListWidget();
};