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

    // "stable" or "beta". Defaults to beta when this build's own version
    // carries a pre-release tag — someone running a beta wants beta news.
    static QString channel();

    // Surfaces any already-known update immediately, then asks the server —
    // once per application run. Reopening the launcher from the Home button
    // reuses the answer from the first check rather than asking again.
    //
    // `force` overrides that, for the menu-driven "check now".
    //
    // The two halves are independent on purpose: how often the server is asked
    // is not how often the user is told. Without the first half, an update
    // found at startup would stop being displayed the moment the launcher was
    // closed and reopened.
    void check(bool force = false);

    // The newest release seen by a previous check, remembered across restarts.
    // Empty when there is none, or when this build has caught up with it.
    static QString knownUpdateVersion();

signals:
    // Emitted only when the published version is strictly newer than this one.
    // `downloadUrl` is the site's /download page, which lists every build.
    void updateAvailable(const QString& version, const QString& title,
                         const QString& downloadUrl);

    // Emitted on every completed check, including "you're up to date" and
    // failures, so a manual check can report something either way.
    void checkFinished(bool foundUpdate, const QString& error);

private:
    void handleReply(class QNetworkReply* reply);
    static void rememberUpdate(const QString& version, const QString& title, const QString& url);
    static void forgetUpdate();

    QNetworkAccessManager* network = nullptr;
    bool inFlight = false;

    // One automatic check per run. The endpoint sends Cache-Control:
    // max-age=300 and a launch is not a frequent event, so there is nothing to
    // gain from asking twice in a session.
    bool checkedThisRun = false;
};
