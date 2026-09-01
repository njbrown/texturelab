#include "catalogservice.h"
#include "mainwindow.h"
#include "systeminfo.h"
#include "telemetry.h"
#include "thememanager.h"
#include "version.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QThread>

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

// Force a consistent dark theme on every platform, independent of the host
// system theme. Loads the design-token theme (Fusion + dark palette + app
// stylesheet) from resources so it works even in a minimal Linux AppImage that
// has no desktop theme plugin — which is why CI builds otherwise render in light
// mode. See src/theme/ and UI_DESIGN_SYSTEM_PRD.md.
static void applyDarkTheme(QApplication& app)
{
    ThemeManager& tm = ThemeManager::instance();
    tm.loadFromResource(":/themes/dark.json");
    tm.setStyleSheetTemplate(":/qss/app.qss");
    tm.setAdsStyleSheetTemplate(":/qss/ads.qss"); // applied to the dock manager by MainWindow
    tm.applyToApplication(app);
}

int main(int argc, char* argv[])
{
    // Read the stored answer before constructing QApplication. Off until the
    // consent prompt has actually been answered — the launcher puts it up on
    // first run and turns collection on from there if the answer is yes.
    const bool crashReportingEnabled = Telemetry::isAllowed();

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

    // Consistent dark UI on every platform, regardless of the host system theme.
    applyDarkTheme(a);

    // Theme hot-reload: live-reloads the theme from the on-disk source files
    // (resources/…) on save, so colors and QSS can be tuned without rebuilding.
    // ON BY DEFAULT in Debug builds (TEXTURELAB_DEV_BUILD); off in Release.
    // `--dev-theme` forces it on in any build; `--no-dev-theme` forces it off.
    bool devTheme = false;
#ifdef TEXTURELAB_DEV_BUILD
    devTheme = true;
#endif
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--dev-theme") == 0)
            devTheme = true;
        else if (std::strcmp(argv[i], "--no-dev-theme") == 0)
            devTheme = false;
    }
    if (devTheme) {
#ifdef TEXTURELAB_SOURCE_RESOURCES
        const QString res = QStringLiteral(TEXTURELAB_SOURCE_RESOURCES);
        ThemeManager::instance().enableHotReload(res + "/themes/dark.json",
                                                 res + "/qss/app.qss.in",
                                                 res + "/qss/ads.qss.in");
        qInfo("Theme hot-reload enabled, watching %s", qPrintable(res));
#else
        qWarning("theme hot-reload requested but TEXTURELAB_SOURCE_RESOURCES not compiled in");
#endif
    }

    // Now applicationDirPath() is valid — init Sentry
    Telemetry::init(crashReportingEnabled);

    // Register RAM/CPU/screen/platform context before anything touches OpenGL.
    // The GPU half needs a current context and is registered later, from
    // TextureRenderer::setup(); this half has to already be on the scope in
    // case we die during GL init itself.
    SystemInfo::reportSystemContext();

    // Launcher index + thumbnail cache. Failure is not fatal: the app runs
    // normally without them, it just has nothing to show in the launcher.
    // Seeds from the legacy recent-files list on first run (LAUNCHER_PRD.md §1.1).
    if (CatalogService::instance().init())
        CatalogService::instance().reconcileAsync();

    // Crash-test hook for verifying Sentry symbolication end-to-end.
    // Must run after Telemetry::init so the crash handler is armed.
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--sentry-crash-test") == 0) {
            // Give the SDK's network transport a moment to spin up before we
            // crash. Crashpad (Win/Linux) uploads out-of-process so this isn't
            // needed there, but the macOS inproc backend must send the event
            // synchronously from the dying process — an instant crash at
            // startup dies before the transport is ready. A real crash happens
            // after the app has been running, so this warm-up is representative.
            QThread::sleep(4);
            sentryCrashTest();
        }
    }

    // Install message handler after Sentry is up so breadcrumbs are captured
    qInstallMessageHandler(qtMessageHandler);

    MainWindow w;

    // The launcher comes up first and MainWindow stays hidden until something
    // is opened or created. Constructing it here is unavoidable — it owns the
    // renderer and the dock layout — but not showing it keeps the empty editor
    // off screen behind the launcher. `--no-launcher` skips straight to the
    // editor, which is what you want when iterating on the editor itself.
    bool useLauncher = true;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--no-launcher") == 0)
            useLauncher = false;
    }

    if (useLauncher) {
        w.showLauncher();
    }
    else {
        w.show();
        w.showMaximized();
    }

    int ret = a.exec();

    // Close the catalog databases here, while Qt's SQL layer is still alive.
    // Leaving it to static destruction crashes on exit (see instance()).
    CatalogService::instance().shutdown();

    Telemetry::close();
    return ret;
}
