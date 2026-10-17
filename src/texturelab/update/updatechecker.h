#pragma once

#include "versioncompare.h"

#include <QObject>
#include <QString>

class QJsonObject;
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

    // What this build is. main() sets it from the compiled-in version,
    // channel, commit count and hash before anything checks.
    static void setCurrentBuild(const appversion::Build& build);
    static appversion::Build currentBuild();

    // "stable" or "beta": this build's own channel, never a setting, so a build
    // can't ask about a channel it isn't on. The server answers "beta" with
    // the newer of the latest beta and the latest stable.
    static QString channel();

    // Asks the server, once per application run. Nothing is remembered across
    // runs: every launch asks afresh, so a pulled or corrected release can't
    // leave a stale notice behind. Reopening the launcher from the Home button
    // doesn't ask again — the launcher is reused, and so is what it shows.
    //
    // `force` overrides that, for the menu-driven "check now".
    void check(bool force = false);

    // What a `data` object from /api/releases/latest means for `current`.
    // Pure, so the channel and ordering rules are testable without a network.
    struct Offer {
        bool found = false;
        QString identifier;
        QString title;
        QString downloadUrl;
        QString error; // set when the response itself was unusable
    };
    static Offer evaluate(const QJsonObject& data, const appversion::Build& current);

signals:
    // Emitted only when the published build is strictly newer than this one.
    // `identifier` is the release's display name ("0.4.0-beta-039faa9");
    // `downloadUrl` is that release's page on the site.
    void updateAvailable(const QString& identifier, const QString& title,
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
