#pragma once

#include <QDialog>

// Asks permission to send crash reports, once per released version.
//
// Says what leaves the machine and what doesn't, because "help improve the app"
// on its own asks the user to agree to something they can't see. All styling
// comes from the theme (see #Consent* in app.qss.in) — no inline colors here.
class CrashConsentDialog : public QDialog {
    Q_OBJECT

public:
    explicit CrashConsentDialog(QWidget* parent = nullptr);

    // Puts the prompt up and records the answer, turning collection on or off
    // to match. Returns what the user chose.
    //
    // Dismissing the dialog counts as declining but still counts as asked: a
    // prompt that reappears every launch until it gets the answer it wants is
    // not a question.
    static bool ask(QWidget* parent);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QWidget* buildFact(const QString& lead, const QString& detail);
};
