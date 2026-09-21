#pragma once
#include <QWidget>

class QVBoxLayout;
class QPushButton;

class AccordionWidget : public QWidget {
    Q_OBJECT

    QString _title;
    QPushButton* headerButton;
    QWidget* contentWidget;
    QVBoxLayout* contentLayout;
    bool _collapsed;

    void updateHeader();

public:
    AccordionWidget(const QString& title, bool startCollapsed = false,
                    QWidget* parent = nullptr);
    void addWidget(QWidget* widget);
};
