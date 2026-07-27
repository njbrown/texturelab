#include "accordionwidget.h"

#include <QPushButton>
#include <QVBoxLayout>

AccordionWidget::AccordionWidget(const QString& title, bool startCollapsed,
                                 QWidget* parent)
    : QWidget(parent), _title(title), _collapsed(startCollapsed)
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 2);
    outerLayout->setSpacing(0);
    this->setLayout(outerLayout);

    headerButton = new QPushButton(this);
    headerButton->setObjectName("AccordionHeader"); // styled in app.qss.in
    headerButton->setFlat(true);
    headerButton->setCursor(Qt::PointingHandCursor);
    outerLayout->addWidget(headerButton);

    contentWidget = new QWidget(this);
    contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentWidget->setLayout(contentLayout);
    outerLayout->addWidget(contentWidget);

    updateHeader();
    contentWidget->setVisible(!_collapsed);

    connect(headerButton, &QPushButton::clicked, this, [this]() {
        _collapsed = !_collapsed;
        updateHeader();
        contentWidget->setVisible(!_collapsed);
    });
}

void AccordionWidget::updateHeader()
{
    QString arrow = _collapsed ? " ▶ " : " ▼ ";
    headerButton->setText(arrow + _title);
}

void AccordionWidget::addWidget(QWidget* widget)
{
    contentLayout->addWidget(widget);
}
