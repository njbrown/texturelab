#include "aboutdialog.h"

#include <QApplication>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("About TextureLab");
    setFixedSize(440, 320);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
}

void AboutDialog::setupUI()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #1e1e1e;
            color: #e0e0e0;
        }
        QLabel {
            color: #e0e0e0;
            background: transparent;
        }
        QPushButton#closeBtn {
            background-color: #3a3a3a;
            color: #e0e0e0;
            border: none;
            border-radius: 4px;
            padding: 6px 20px;
            font-size: 13px;
        }
        QPushButton#closeBtn:hover {
            background-color: #4a4a4a;
        }
        QPushButton#closeBtn:pressed {
            background-color: #2a2a2a;
        }
    )");

    auto outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Header band
    auto header = new QWidget();
    header->setFixedHeight(110);
    header->setStyleSheet("background: transparent;");

    auto headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(28, 0, 28, 0);
    headerLayout->setSpacing(20);

    auto logoLabel = new QLabel();
    QPixmap logo(":/icons/logo.png");
    if (!logo.isNull()) {
        logoLabel->setPixmap(
            logo.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    logoLabel->setFixedSize(64, 64);
    headerLayout->addWidget(logoLabel);

    auto titleBlock = new QVBoxLayout();
    titleBlock->setSpacing(4);

    auto nameLabel = new QLabel("TextureLab");
    nameLabel->setStyleSheet(
        "color: #ffffff; font-size: 26px; font-weight: bold;");
    titleBlock->addWidget(nameLabel);

    auto tagLabel = new QLabel("Procedural Texture Authoring");
    tagLabel->setStyleSheet("color: #a0b4d0; font-size: 12px;");
    titleBlock->addWidget(tagLabel);

    headerLayout->addLayout(titleBlock);
    headerLayout->addStretch();

    outerLayout->addWidget(header);

    // Body
    auto body = new QWidget();
    auto bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(28, 22, 28, 20);
    bodyLayout->setSpacing(10);

    QString version = QCoreApplication::applicationVersion();
    auto versionLabel = new QLabel(QString("Version %1").arg(version));
    versionLabel->setStyleSheet("font-size: 13px; color: #b0b0b0;");
    bodyLayout->addWidget(versionLabel);

    auto separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color: #333333;");
    bodyLayout->addWidget(separator);

    auto descLabel = new QLabel(
        "A node-based texture creation tool for game artists and developers.");
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("font-size: 13px; color: #c0c0c0; line-height: 1.4;");
    bodyLayout->addWidget(descLabel);

    auto linkLabel = new QLabel(
        "<a href='https://github.com/njbrown/texturelab' "
        "style='color:#5b9bd5;'>github.com/njbrown/texturelab</a>");
    linkLabel->setOpenExternalLinks(true);
    linkLabel->setStyleSheet("font-size: 12px;");
    bodyLayout->addWidget(linkLabel);

    bodyLayout->addStretch();

    auto footerLayout = new QHBoxLayout();
    auto copyrightLabel = new QLabel("© Nicolas Brown");
    copyrightLabel->setStyleSheet("font-size: 11px; color: #666666;");
    footerLayout->addWidget(copyrightLabel);
    footerLayout->addStretch();

    auto closeBtn = new QPushButton("Close");
    closeBtn->setObjectName("closeBtn");
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footerLayout->addWidget(closeBtn);

    bodyLayout->addLayout(footerLayout);
    outerLayout->addWidget(body);
}
