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
constexpr const char* kApiBase = "http://localhost:3333";

constexpr const char* kEnabledKey = "updateCheck";

// The last release the server told us about. Remembered so the launcher can say
// so on every open, not just during the minutes after a check completes.
constexpr const char* kKnownVersionKey = "updateKnownVersion";
constexpr const char* kKnownTitleKey = "updateKnownTitle";
constexpr const char* kKnownUrlKey = "updateKnownUrl";

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
    const QString stored = appSettings().value(QStringLiteral("updateChannel")).toString();
    if (stored == QLatin1String("stable") || stored == QLatin1String("beta"))
        return stored;

    // A pre-release tag on our own version means this is a beta build, and its
    // user is better served by beta releases than by being told nothing exists.
    const QString self = appversion::normalize(QCoreApplication::applicationVersion());
    return self.contains(QLatin1Char('-')) ? QStringLiteral("beta") : QStringLiteral("stable");
}

QString UpdateChecker::platformKey()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("mac");
#else
    return QStringLiteral("linux");
#endif
}

QString UpdateChecker::knownUpdateVersion()
{
    const QString version = appSettings().value(QLatin1String(kKnownVersionKey)).toString();
    if (version.isEmpty())
        return QString();

    // A remembered update the running build has caught up with is stale — the
    // user updated, and nothing should still be nagging them.
    if (!appversion::isNewer(version, QCoreApplication::applicationVersion()))
        return QString();

    return version;
}

void UpdateChecker::rememberUpdate(const QString& version, const QString& title,
                                   const QString& url)
{
    QSettings settings = appSettings();
    settings.setValue(QLatin1String(kKnownVersionKey), version);
    settings.setValue(QLatin1String(kKnownTitleKey), title);
    settings.setValue(QLatin1String(kKnownUrlKey), url);
}

void UpdateChecker::forgetUpdate()
{
    QSettings settings = appSettings();
    settings.remove(QLatin1String(kKnownVersionKey));
    settings.remove(QLatin1String(kKnownTitleKey));
    settings.remove(QLatin1String(kKnownUrlKey));
}

void UpdateChecker::check(bool force)
{
    if (!isEnabled() || inFlight)
        return;

    QSettings settings = appSettings();

    // Say what we already know before deciding whether to ask again.
    const QString known = knownUpdateVersion();
    if (!known.isEmpty()) {
        emit updateAvailable(known, settings.value(QLatin1String(kKnownTitleKey)).toString(),
                             settings.value(QLatin1String(kKnownUrlKey)).toString());
    }

    // Once per run unless explicitly forced.
    if (!force && checkedThisRun)
        return;

    QUrl url(apiBase() + QStringLiteral("/api/releases/latest"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("channel"), channel());
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("TextureLab/%1").arg(QCoreApplication::applicationVersion()));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(8000);

    if (!network)
        network = new QNetworkAccessManager(this);

    inFlight = true;
    checkedThisRun = true;

    QNetworkReply* reply = network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReply(reply); });
}

void UpdateChecker::handleReply(QNetworkReply* reply)
{
    inFlight = false;
    reply->deleteLater();

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
        emit checkFinished(false, QStringLiteral("Malformed response from the update server"));
        return;
    }

    // 404 with {"error": ...} is a legitimate answer: the channel has no
    // published release yet.
    const QJsonObject root = doc.object();
    if (!root.value(QStringLiteral("data")).isObject()) {
        const QString error = root.value(QStringLiteral("error")).toString();
        emit checkFinished(false, error);
        return;
    }

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QString version = data.value(QStringLiteral("version")).toString();
    if (version.isEmpty()) {
        emit checkFinished(false, QStringLiteral("Update server returned no version"));
        return;
    }

    const QString current = QCoreApplication::applicationVersion();
    if (!appversion::isNewer(version, current)) {
        // Up to date now — drop anything remembered from before, so a notice
        // can't outlive the update it referred to.
        forgetUpdate();
        emit checkFinished(false, QString());
        return;
    }

    const QJsonObject downloads = data.value(QStringLiteral("downloads")).toObject();
    QString downloadUrl = downloads.value(platformKey()).toString();

    // No build for this platform yet — still worth telling them, pointed at the
    // page rather than at nothing.
    if (downloadUrl.isEmpty())
        downloadUrl = apiBase() + QStringLiteral("/#download");

    Telemetry::breadcrumb("update", "found " + version.toStdString());

    rememberUpdate(version, data.value(QStringLiteral("title")).toString(), downloadUrl);

    emit updateAvailable(version, data.value(QStringLiteral("title")).toString(), downloadUrl);
    emit checkFinished(true, QString());
}
