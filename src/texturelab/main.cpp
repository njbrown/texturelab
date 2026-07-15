#include "mainwindow.h"
#include "telemetry.h"
#include "version.h"

#include <QApplication>
#include <QSettings>
#include <QSurfaceFormat>

#include <cstring>

// Hints that a dedicated GPU should be used whenever possible
// https://stackoverflow.com/a/39047129/991834
#ifdef Q_OS_WIN
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

// Forward qWarning/qCritical as Sentry breadcrumbs; qFatal as a captured event
// so we get context before Crashpad's abort handler fires.
static void qtMessageHandler(QtMsgType type, const QMessageLogContext& /*ctx*/,
                             const QString& msg)
{
    switch (type) {
    case QtDebugMsg:
        break;
    case QtInfoMsg:
        Telemetry::breadcrumb("qt.info", msg.toStdString());
        break;
    case QtWarningMsg:
        Telemetry::breadcrumb("qt.warning", msg.toStdString());
        break;
    case QtCriticalMsg:
        Telemetry::breadcrumb("qt.critical", msg.toStdString());
        break;
    case QtFatalMsg:
        Telemetry::captureException("qFatal: " + msg.toStdString());
        // Allow default abort() so Crashpad captures the minidump
        abort();
    }
}

#if defined(_MSC_VER)
#define TL_NOINLINE __declspec(noinline)
#else
#define TL_NOINLINE __attribute__((noinline))
#endif

// Deliberately dereference a null pointer so Crashpad captures a minidump.
// Kept in a named, non-inlined function so the symbolicated Sentry stack trace
// shows a recognizable frame. Triggered only via the --sentry-crash-test flag;
// remove this hook once symbolication is confirmed in production.
TL_NOINLINE static void sentryCrashTest()
{
    volatile int* p = nullptr;
    *p = 0xC0FFEE;
}

int main(int argc, char* argv[])
{
    // Read opt-out before constructing QApplication so we can use QSettings
    // with an explicit scope (no org/app name set yet).
    QSettings settings(QSettings::UserScope, "texturelab", "texturelab");
    bool crashReportingEnabled =
        settings.value("crashReporting", true).toBool();

    // Init Sentry before QApplication; resolves paths via Qt helpers after
    // QCoreApplication is available (handler_path needs applicationDirPath).
    // We pass a temporary QCoreApplication for path resolution, then tear it
    // down before the real QApplication is constructed.
    //
    // Actually: sentry_options_set_handler_path / database_path only need the
    // strings — we can derive them from argv[0] or defer to after QApplication.
    // Simplest: init Sentry after QApplication (Crashpad handler is separate
    // process anyway so it doesn't need QApplication to be alive).

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 2);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    a.setOrganizationName("texturelab");
    a.setApplicationName("texturelab");
    a.setApplicationVersion(QString(TEXTURELAB_VERSION) + "+" + TEXTURELAB_BUILD_HASH);

    // Now applicationDirPath() is valid — init Sentry
    Telemetry::init(crashReportingEnabled);

    // Crash-test hook for verifying Sentry symbolication end-to-end.
    // Must run after Telemetry::init so Crashpad is armed to catch it.
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--sentry-crash-test") == 0)
            sentryCrashTest();
    }

    // Install message handler after Sentry is up so breadcrumbs are captured
    qInstallMessageHandler(qtMessageHandler);

    MainWindow w;
    w.show();
    w.showMaximized();

    int ret = a.exec();
    Telemetry::close();
    return ret;
}
