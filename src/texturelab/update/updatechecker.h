#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;

// Asks texturelab.io whether a newer build exists.
//
// Read-only and fire-and-forget: one GET to a public, unauthenticated endpoint,
// no payload, no identifiers beyond what any HTTP request carries. It never
// downloads or installs anything — finding an update just surfaces a link the
// user can choose to click.
//
// Opt-out lives in QSettings under "updateCheck", matching how crash reporting
// is handled. Off means no request is made at all, not a discarded response.
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker() override;

    // Endpoint base, without a trailing slash. Hardcoded in updatechecker.cpp;
    // point it at a dev server by editing that constant and rebuilding.
    static QString apiBase();

    static bool isEnabled();
    static void setEnabled(bool enabled);

    // "stable" or "beta", derived from this build's own version alone: a
    // pre-release tag means beta — someone running a beta wants beta news.
    // Not a setting, so a build can never ask about a channel it isn't on.
    static QString channel();

    // Asks the server, once per application run. Nothing is remembered across
    // runs: every launch asks afresh, so a pulled or corrected release can't
    // leave a stale notice behind. Reopening the launcher from the Home button
    // doesn't ask again — the launcher is reused, and so is what it shows.
    //
    // `force` overrides that, for the menu-driven "check now".
    void check(bool force = false);

signals:
    // Emitted only when the published version is strictly newer than this one.
    // `downloadUrl` is the site's /download page, which lists every build.
    void updateAvailable(const QString& version, const QString& title,
                         const QString& downloadUrl);

    // Emitted on every completed check, including "you're up to date" and
    // failures, so a manual check can report something either way. An empty
    // `error` with `foundUpdate` false means the server answered and there is
    // nothing newer — including a 404, which is a channel with no release.
    void checkFinished(bool foundUpdate, const QString& error);

private:
    void handleReply(class QNetworkReply* reply);

    QNetworkAccessManager* network = nullptr;
    bool inFlight = false;

    // One automatic check per run. The endpoint sends Cache-Control:
    // max-age=300 and a launch is not a frequent event, so there is nothing to
    // gain from asking twice in a session.
    bool checkedThisRun = false;
};
