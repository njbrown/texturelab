#include "updatechecker.h"

#include "telemetry.h"
#include "versioncompare.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>

namespace {

// The update endpoint. Hardcoded on purpose — no build flag, no environment
// override — so what the app asks is whatever this one line says.
//
// !!! Currently pointed at the local dev server. Set this back to
// !!! "https://texturelab.io" before cutting a release: a shipped build pointed
// !!! at localhost never reaches anything and silently reports no updates.
// constexpr const char* kApiBase = "http://localhost:3333";
constexpr const char* kApiBase = "https://texturelab.io";

constexpr const char* kEnabledKey = "updateCheck";

QSettings appSettings()
{
    return QSettings(QSettings::UserScope, "texturelab", "texturelab");
}

// Set once by main() from the compiled-in build info. Empty until then, which
// isValid() rejects — an unset build never claims an update.
appversion::Build& runningBuild()
{
    static appversion::Build build;
    return build;
}

} // namespace

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent) {}

UpdateChecker::~UpdateChecker() = default;

QString UpdateChecker::apiBase()
{
    // Trailing slashes are trimmed rather than assumed absent: the request path
    // is appended directly, and "…3333/" + "/api/…" is a double-slashed path
    // that some routers answer with a 404.
    QString base = QString::fromLatin1(kApiBase).trimmed();
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);

    return base;
}

bool UpdateChecker::isEnabled()
{
    return appSettings().value(QLatin1String(kEnabledKey), true).toBool();
}

void UpdateChecker::setEnabled(bool enabled)
{
    QSettings settings = appSettings();
    settings.setValue(QLatin1String(kEnabledKey), enabled);
}

void UpdateChecker::setCurrentBuild(const appversion::Build& build)
{
    runningBuild() = build;
}

appversion::Build UpdateChecker::currentBuild()
{
    return runningBuild();
}

QString UpdateChecker::channel()
{
    return runningBuild().channel;
}

void UpdateChecker::check(bool force)
{
    // Once per run unless explicitly forced.
    if (!isEnabled() || inFlight || (!force && checkedThisRun))
        return;

    // A build that doesn't know what it is (no git at configure time) can't be
    // ordered against anything, so there is nothing worth asking.
    if (!runningBuild().isValid()) {
        emit checkFinished(false, QStringLiteral("This build has no version information"));
        return;
    }

    QUrl url(apiBase() + QStringLiteral("/api/releases/latest"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("channel"), channel());
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("TextureLab/%1 (build %2)")
                          .arg(runningBuild().identifier())
                          .arg(runningBuild().number));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(8000);

    if (!network)
        network = new QNetworkAccessManager(this);

    inFlight = true;
    checkedThisRun = true;

    QNetworkReply* reply = network->get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply]() { handleReply(reply); });
}

void UpdateChecker::handleReply(QNetworkReply* reply)
{
    inFlight = false;
    reply->deleteLater();

    // 404 is the server saying the channel has no published release (a pulled
    // release looks the same), so there is nothing newer to offer. Qt reports it
    // as an error; it is an answer.
    const int status =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 404) {
        emit checkFinished(false, QString());
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        // Being offline is the normal case, not an incident: report it to the
        // caller and leave no trace beyond a breadcrumb.
        const QString error = reply->errorString();
        Telemetry::breadcrumb("update", "check failed: " + error.toStdString());
        emit checkFinished(false, error);
        return;
    }

    const QByteArray body = reply->readAll();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit checkFinished(
            false, QStringLiteral("Malformed response from the update server"));
        return;
    }

    const QJsonObject root = doc.object();
    if (!root.value(QStringLiteral("data")).isObject()) {
        const QString error = root.value(QStringLiteral("error")).toString();
        emit checkFinished(false, error);
        return;
    }

    const Offer offer =
        evaluate(root.value(QStringLiteral("data")).toObject(), runningBuild());
    if (!offer.found) {
        emit checkFinished(false, offer.error);
        return;
    }

    Telemetry::breadcrumb("update", "found " + offer.identifier.toStdString());

    emit updateAvailable(offer.identifier, offer.title, offer.downloadUrl);
    emit checkFinished(true, QString());
}

UpdateChecker::Offer UpdateChecker::evaluate(const QJsonObject& data,
                                             const appversion::Build& current)
{
    Offer offer;

    appversion::Build latest;
    latest.version = data.value(QStringLiteral("version")).toString();
    latest.channel = data.value(QStringLiteral("channel")).toString();
    latest.number = data.value(QStringLiteral("build")).toInt();
    latest.commit = data.value(QStringLiteral("commit")).toString();
    if (!latest.isValid()) {
        offer.error = QStringLiteral("Update server returned no usable version");
        return offer;
    }

    // The server answers the beta channel with a stable release when that is
    // newer, so a beta build accepts either. A stable build accepts only
    // stable: whatever the server sends, a stable user is never moved onto a
    // beta by a misconfigured release.
    const bool channelOk = latest.channel == current.channel ||
                           latest.channel == QLatin1String("stable");
    if (!channelOk || !appversion::isNewer(latest, current))
        return offer;

    // The release's own page, so a beta user lands on the beta they were told
    // about rather than on /download's headline stable. Only taken when it
    // points back at the same site; otherwise the generic download page.
    offer.downloadUrl = data.value(QStringLiteral("pageUrl")).toString();
    if (!offer.downloadUrl.startsWith(apiBase() + QLatin1Char('/')))
        offer.downloadUrl = apiBase() + QStringLiteral("/download");

    offer.found = true;
    offer.identifier = latest.identifier();
    offer.title = data.value(QStringLiteral("title")).toString();
    return offer;
}
