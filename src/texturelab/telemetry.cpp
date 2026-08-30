#include "telemetry.h"
#include "version.h"

#include <sentry.h>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QString>

static bool g_enabled = false;

namespace {

// Explicit scope: init() runs before the organization and application names are
// set on QApplication, so the default constructor would read the wrong file.
QSettings consentSettings()
{
    return QSettings(QSettings::UserScope, QStringLiteral("texturelab"),
                     QStringLiteral("texturelab"));
}

constexpr const char* kAllowedKey = "crashReporting";
constexpr const char* kAskedVersionKey = "crashReportingConsentVersion";

} // namespace

void Telemetry::init(bool enabled)
{
    // TEXTURELAB_SENTRY_DSN is injected at compile time from CMake.
    // Fall back to the hardcoded DSN for local dev builds.
    const char* dsn = TEXTURELAB_SENTRY_DSN;
    if (!dsn || dsn[0] == '\0')
        dsn = "https://93029fba3adb2d1782affd712d013a2b@o216182.ingest.us.sentry.io/4511628329680896";
    if (!enabled) {
        g_enabled = false;
        return;
    }

    sentry_options_t* options = sentry_options_new();

    sentry_options_set_dsn(options, dsn);
    sentry_options_set_release(options, "texturelab@" TEXTURELAB_VERSION "+" TEXTURELAB_BUILD_HASH);
    sentry_options_set_debug(options, 0);

    // Writable per-user directory for Crashpad's crash database
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/sentry";
    QDir().mkpath(dataDir);
    sentry_options_set_database_path(options, dataDir.toStdString().c_str());

    // crashpad_handler lives next to the texturelab executable
    QString handlerPath = QCoreApplication::applicationDirPath()
#ifdef Q_OS_WIN
                          + "/crashpad_handler.exe";
#else
                          + "/crashpad_handler";
#endif
    sentry_options_set_handler_path(options, handlerPath.toStdString().c_str());

    sentry_init(options);
    g_enabled = true;
}

void Telemetry::close()
{
    if (g_enabled)
        sentry_close();
}

void Telemetry::setEnabled(bool enabled)
{
    if (enabled == g_enabled)
        return;

    if (enabled) {
        init(true);
    }
    else {
        sentry_close();
        g_enabled = false;
    }
}

bool Telemetry::isEnabled()
{
    return g_enabled;
}

bool Telemetry::isAllowed()
{
    // Defaults to off: an install that has never answered the prompt has not
    // agreed to anything, and a crash before the first answer is the one case
    // where staying quiet costs the least.
    return consentSettings().value(QLatin1String(kAllowedKey), false).toBool();
}

bool Telemetry::consentNeeded()
{
    const QString asked =
        consentSettings().value(QLatin1String(kAskedVersionKey)).toString();
    return asked != QLatin1String(TEXTURELAB_VERSION);
}

void Telemetry::recordConsent(bool allowed)
{
    QSettings settings = consentSettings();
    settings.setValue(QLatin1String(kAllowedKey), allowed);
    settings.setValue(QLatin1String(kAskedVersionKey), QLatin1String(TEXTURELAB_VERSION));
}

void Telemetry::breadcrumb(const char* category, const std::string& message)
{
    if (!g_enabled)
        return;

    sentry_value_t crumb = sentry_value_new_breadcrumb("default", message.c_str());
    sentry_value_set_by_key(crumb, "category", sentry_value_new_string(category));
    sentry_add_breadcrumb(crumb);
}

void Telemetry::captureException(const std::string& message)
{
    if (!g_enabled)
        return;

    sentry_value_t event = sentry_value_new_event();
    sentry_value_t exc = sentry_value_new_exception("Exception", message.c_str());
    sentry_event_add_exception(event, exc);
    sentry_capture_event(event);
}
