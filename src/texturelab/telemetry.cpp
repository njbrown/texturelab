#include "telemetry.h"
#include "version.h"

#include <sentry.h>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QString>

#include <cstdint>
#include <map>
#include <mutex>
#include <type_traits>

static bool g_enabled = false;

// Tags and contexts are recorded even while reporting is off, and replayed
// when it's turned on. Device/GPU info is only gathered once, at startup and
// GL init; without this, a user who opts in mid-session would send crashes
// with none of it.
static std::mutex g_scopeMutex;
static std::map<std::string, std::string> g_tags;
static std::map<std::string, Telemetry::Fields> g_contexts;

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

// Consent is asked once per version and channel ("0.4.0-beta"), not per build:
// every beta of a cycle shares one answer.
constexpr const char* kConsentVersion = TEXTURELAB_VERSION "-" TEXTURELAB_CHANNEL;

// Fields -> sentry object. Kept in one place so breadcrumbs, contexts and
// events all serialize identically.
sentry_value_t toSentryObject(const Telemetry::Fields& fields)
{
    sentry_value_t obj = sentry_value_new_object();
    for (const auto& [key, value] : fields) {
        sentry_value_t v = std::visit(
            [](const auto& held) -> sentry_value_t {
                using T = std::decay_t<decltype(held)>;
                if constexpr (std::is_same_v<T, std::string>)
                    return sentry_value_new_string(held.c_str());
                else if constexpr (std::is_same_v<T, int64_t>)
                    // sentry_value_new_int32 would silently truncate byte
                    // counts, so anything that doesn't fit goes as a double.
                    return held >= INT32_MIN && held <= INT32_MAX
                               ? sentry_value_new_int32((int32_t)held)
                               : sentry_value_new_double((double)held);
                else if constexpr (std::is_same_v<T, double>)
                    return sentry_value_new_double(held);
                else
                    return sentry_value_new_bool(held);
            },
            value);
        sentry_value_set_by_key(obj, key.c_str(), v);
    }
    return obj;
}

} // namespace

void Telemetry::init(bool enabled)
{
    // TEXTURELAB_SENTRY_DSN is injected at compile time from CMake. Local and
    // fork builds have none and send nothing; they must not report into the
    // production project. Pass -DTEXTURELAB_SENTRY_DSN=... to test locally.
    const char* dsn = TEXTURELAB_SENTRY_DSN;
    if (!dsn || dsn[0] == '\0')
        enabled = false;
    if (!enabled) {
        g_enabled = false;
        return;
    }

    sentry_options_t* options = sentry_options_new();

    sentry_options_set_dsn(options, dsn);
    sentry_options_set_release(options, "texturelab@" TEXTURELAB_VERSION "-" TEXTURELAB_CHANNEL
                                        "-" TEXTURELAB_BUILD_HASH);
    sentry_options_set_environment(options, TEXTURELAB_SENTRY_ENVIRONMENT);
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

    // Restore the scope recorded while reporting was off (see g_tags).
    std::lock_guard<std::mutex> lock(g_scopeMutex);
    for (const auto& [key, value] : g_tags)
        sentry_set_tag(key.c_str(), value.c_str());
    for (const auto& [name, fields] : g_contexts)
        sentry_set_context(name.c_str(), toSentryObject(fields));
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
    return asked != QLatin1String(kConsentVersion);
}

void Telemetry::recordConsent(bool allowed)
{
    QSettings settings = consentSettings();
    settings.setValue(QLatin1String(kAllowedKey), allowed);
    settings.setValue(QLatin1String(kAskedVersionKey), QLatin1String(kConsentVersion));
}

void Telemetry::breadcrumb(const char* category, const std::string& message)
{
    Telemetry::breadcrumb(category, message, {});
}

void Telemetry::breadcrumb(const char* category, const std::string& message,
                           const Fields& data)
{
    if (!g_enabled)
        return;

    sentry_value_t crumb = sentry_value_new_breadcrumb("default", message.c_str());
    sentry_value_set_by_key(crumb, "category", sentry_value_new_string(category));
    if (!data.empty())
        sentry_value_set_by_key(crumb, "data", toSentryObject(data));
    sentry_add_breadcrumb(crumb);
}

void Telemetry::setTag(const char* key, const std::string& value)
{
    {
        std::lock_guard<std::mutex> lock(g_scopeMutex);
        g_tags[key] = value;
    }
    if (!g_enabled)
        return;

    sentry_set_tag(key, value.c_str());
}

void Telemetry::setContext(const char* name, const Fields& fields)
{
    {
        std::lock_guard<std::mutex> lock(g_scopeMutex);
        g_contexts[name] = fields;
    }
    if (!g_enabled)
        return;

    sentry_set_context(name, toSentryObject(fields));
}

void Telemetry::captureException(const std::string& message)
{
    Telemetry::captureException(message, {});
}

void Telemetry::captureException(const std::string& message, const Fields& data)
{
    if (!g_enabled)
        return;

    sentry_value_t event = sentry_value_new_event();
    sentry_value_t exc = sentry_value_new_exception("Exception", message.c_str());
    sentry_event_add_exception(event, exc);
    if (!data.empty())
        sentry_value_set_by_key(event, "extra", toSentryObject(data));
    sentry_capture_event(event);
}
