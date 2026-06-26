#include "mainwindow.h"
#include "telemetry.h"

#include <QApplication>
#include <QSettings>
#include <QSurfaceFormat>

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
    a.setApplicationVersion(TEXTURELAB_VERSION);

    // Now applicationDirPath() is valid — init Sentry
    Telemetry::init(crashReportingEnabled);

    // Install message handler after Sentry is up so breadcrumbs are captured
    qInstallMessageHandler(qtMessageHandler);

    MainWindow w;
    w.show();
    w.showMaximized();

    int ret = a.exec();
    Telemetry::close();
    return ret;
}
