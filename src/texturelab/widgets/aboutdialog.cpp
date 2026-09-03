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
    // Base widget/dialog/button styling comes from the global theme (app.qss.in);
    // only the About-specific label typography is set here (see #About* rules).

    auto outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Header band
    auto header = new QWidget();
    header->setFixedHeight(110);

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
    nameLabel->setObjectName("AboutTitle");
    titleBlock->addWidget(nameLabel);

    auto tagLabel = new QLabel("Procedural Texture Authoring");
    tagLabel->setObjectName("AboutTag");
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
    versionLabel->setObjectName("AboutVersion");
    bodyLayout->addWidget(versionLabel);

    auto separator = new QFrame();
    separator->setObjectName("AboutSeparator");
    separator->setFrameShape(QFrame::HLine);
    bodyLayout->addWidget(separator);

    auto descLabel = new QLabel(
        "A node-based texture creation tool for game artists and developers.");
    descLabel->setWordWrap(true);
    descLabel->setObjectName("AboutDesc");
    bodyLayout->addWidget(descLabel);

    auto linkLabel = new QLabel(
        "<a href='https://github.com/njbrown/texturelab'>"
        "github.com/njbrown/texturelab</a>");
    linkLabel->setOpenExternalLinks(true);
    linkLabel->setObjectName("AboutLink");
    bodyLayout->addWidget(linkLabel);

    bodyLayout->addStretch();

    auto footerLayout = new QHBoxLayout();
    auto copyrightLabel = new QLabel("© Nicolas Brown");
    copyrightLabel->setObjectName("AboutCopyright");
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
