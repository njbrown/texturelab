#include "updatechecker.h"

#include "telemetry.h"
#include "versioncompare.h"

#include <QCoreApplication>
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

QString UpdateChecker::channel()
{
    // A pre-release tag on our own version means this is a beta build, and its
    // user is better served by beta releases than by being told nothing exists.
    const QString self =
        appversion::normalize(QCoreApplication::applicationVersion());
    return self.contains(QLatin1Char('-')) ? QStringLiteral("beta")
                                           : QStringLiteral("stable");
}

void UpdateChecker::check(bool force)
{
    // Once per run unless explicitly forced.
    if (!isEnabled() || inFlight || (!force && checkedThisRun))
        return;

    QUrl url(apiBase() + QStringLiteral("/api/releases/latest"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("channel"), channel());
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("TextureLab/%1")
                          .arg(QCoreApplication::applicationVersion()));
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

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QString version = data.value(QStringLiteral("version")).toString();
    if (version.isEmpty()) {
        emit checkFinished(false,
                           QStringLiteral("Update server returned no version"));
        return;
    }

    const QString current = QCoreApplication::applicationVersion();
    if (!appversion::isNewer(version, current)) {
        emit checkFinished(false, QString());
        return;
    }

    // Always the site's download page rather than a per-platform binary URL:
    // it lists every build, so it stays correct when the server has no artifact
    // for this platform yet, and the user lands somewhere that explains itself.
    const QString downloadUrl = apiBase() + QStringLiteral("/download");

    Telemetry::breadcrumb("update", "found " + version.toStdString());

    emit updateAvailable(
        version, data.value(QStringLiteral("title")).toString(), downloadUrl);
    emit checkFinished(true, QString());
}
