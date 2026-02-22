#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QSharedPointer>
#include <QString>

class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

class TextureProject;
typedef QSharedPointer<TextureProject> TextureProjectPtr;

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(QWidget* parent = nullptr);
    ~ExportDialog();

    void setProject(TextureProjectPtr project);

    QString getExportDestination() const { return exportDestination; }
    QString getExportPattern() const;

private slots:
    void onChooseDestination();
    void onResetPattern();
    void onOk();

private:
    void setupUI();
    void updateDestinationDisplay();

    TextureProjectPtr project;
    QString exportDestination;

    // UI components
    QLabel* destinationLabel;
    QPushButton* chooseDestinationBtn;
    QLineEdit* patternEdit;
    QPushButton* resetPatternBtn;
    QPushButton* okBtn;
    QPushButton* cancelBtn;
};

#endif // EXPORTDIALOG_H
