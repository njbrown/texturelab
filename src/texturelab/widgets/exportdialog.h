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
    QString getExportPattern() const { return exportPattern; }

signals:
    void exportRequested(const QString& destination, const QString& pattern);

private slots:
    void onChooseDestination();
    void onResetPattern();
    void onExport();

private:
    void setupUI();
    void updateDestinationDisplay();

    TextureProjectPtr project;
    QString exportDestination;
    QString exportPattern;

    // UI components
    QLabel* destinationLabel;
    QPushButton* chooseDestinationBtn;
    QLineEdit* patternEdit;
    QPushButton* resetPatternBtn;
    QPushButton* exportBtn;
    QPushButton* cancelBtn;
};

#endif // EXPORTDIALOG_H
