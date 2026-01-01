#include "exportdialog.h"
#include "../models.h"
#include "../project.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

ExportDialog::ExportDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Export Settings");
    setModal(true);
    setMinimumWidth(500);

    setupUI();
}

ExportDialog::~ExportDialog() {}

void ExportDialog::setProject(TextureProjectPtr project)
{
    this->project = project;
    if (project) {
        if (patternEdit) {
            patternEdit->setText(project->exportFilePattern);
        }
        exportDestination = project->exportDestination;
        updateDestinationDisplay();
    }
}

QString ExportDialog::getExportPattern() const
{
    return project ? project->exportFilePattern : "${project}_${name}";
}

void ExportDialog::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // Destination section
    auto destLabel = new QLabel("<b>Destination:</b>");
    mainLayout->addWidget(destLabel);

    auto destLayout = new QHBoxLayout();
    destinationLabel = new QLabel("No destination selected");
    destinationLabel->setStyleSheet("QLabel { padding: 5px; border-radius: "
                                    "3px; }");
    destinationLabel->setWordWrap(true);
    destLayout->addWidget(destinationLabel, 1);

    chooseDestinationBtn = new QPushButton("Choose Folder");
    connect(chooseDestinationBtn, &QPushButton::clicked, this,
            &ExportDialog::onChooseDestination);
    destLayout->addWidget(chooseDestinationBtn);

    mainLayout->addLayout(destLayout);

    // Pattern section
    auto patternLabel = new QLabel("<b>Pattern:</b>");
    mainLayout->addWidget(patternLabel);

    auto patternLayout = new QHBoxLayout();
    patternEdit = new QLineEdit("${project}_${name}");
    patternLayout->addWidget(patternEdit, 1);

    resetPatternBtn = new QPushButton("Reset");
    connect(resetPatternBtn, &QPushButton::clicked, this,
            &ExportDialog::onResetPattern);
    patternLayout->addWidget(resetPatternBtn);

    mainLayout->addLayout(patternLayout);

    // Help text
    auto helpLabel = new QLabel(
        "<small><i>${project} - Project Name<br>${name} - Output Node "
        "Name</i></small>");
    helpLabel->setStyleSheet("QLabel { color: #666; }");
    mainLayout->addWidget(helpLabel);

    // Spacer
    mainLayout->addStretch();

    // Buttons
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    cancelBtn = new QPushButton("Cancel");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);

    okBtn = new QPushButton("OK");
    okBtn->setDefault(true);
    connect(okBtn, &QPushButton::clicked, this, &ExportDialog::onOk);
    buttonLayout->addWidget(okBtn);

    mainLayout->addLayout(buttonLayout);
}

void ExportDialog::updateDestinationDisplay()
{
    if (exportDestination.isEmpty()) {
        destinationLabel->setText("No destination selected");
        destinationLabel->setStyleSheet(
            "QLabel { padding: 5px; background-color: #f0f0f0; border-radius: "
            "3px; color: #999; }");
        chooseDestinationBtn->setText("Choose Folder");
    }
    else {
        destinationLabel->setText(exportDestination);
        destinationLabel->setStyleSheet(
            "QLabel { padding: 5px; background-color: #e8f5e9; border-radius: "
            "3px; }");
        chooseDestinationBtn->setText("...");
    }
}

void ExportDialog::onChooseDestination()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Select Export Destination", exportDestination,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        exportDestination = dir;
        if (project) {
            project->exportDestination = dir;
        }
        updateDestinationDisplay();
    }
}

void ExportDialog::onResetPattern()
{
    QString defaultPattern = "${project}_${name}";
    patternEdit->setText(defaultPattern);
    if (project) {
        project->exportFilePattern = defaultPattern;
    }
}

void ExportDialog::onOk()
{
    // Get current pattern from the input
    QString pattern = patternEdit->text().trimmed();

    // Validate pattern
    if (pattern.isEmpty()) {
        QMessageBox::warning(this, "Invalid Pattern",
                             "Please specify an export pattern.");
        return;
    }

    // Save pattern to project
    if (project) {
        project->exportFilePattern = pattern;
    }

    // Close dialog
    accept();
}
