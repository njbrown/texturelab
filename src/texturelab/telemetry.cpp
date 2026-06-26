#include "telemetry.h"

#include <sentry.h>

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

static bool g_enabled = false;

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
    sentry_options_set_release(options, "texturelab@" TEXTURELAB_VERSION);
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
