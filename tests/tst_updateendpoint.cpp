#include "updatechecker.h"

#include "telemetry.h"

#include <QCoreApplication>
#include <QSettings>
#include <QUrl>
#include <QtTest>

// UpdateChecker leaves breadcrumbs; the real Telemetry pulls in Sentry and a
// generated version header, neither of which this test has an opinion about.
// Stubbing the one entry point it uses keeps the test to the code under test.
namespace Telemetry {
void breadcrumb(const char*, const std::string&) {}
} // namespace Telemetry

// Covers what decides *which server gets asked* and on what channel — the
// question you have when a dev server sees no traffic. The endpoint itself is
// hardcoded, so the interesting assertions are that it stays well-formed and
// that nothing outside the source can redirect it.
class TestUpdateEndpoint : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void endpointIsAWellFormedAbsoluteUrl();
    void endpointIgnoresTheEnvironment();

    void channelFollowsOwnVersion();
    void channelSettingWinsOverVersion();
    void nonsenseChannelSettingIsIgnored();

    void remembersAKnownUpdateAcrossRestarts();
    void forgetsAKnownUpdateOnceTheBuildCatchesUp();
};

void TestUpdateEndpoint::initTestCase()
{
    // Keep every settings write inside the test's own scope rather than the
    // developer's real config.
    QStandardPaths::setTestModeEnabled(true);
}

void TestUpdateEndpoint::cleanup()
{
    qunsetenv("TEXTURELAB_API_BASE");

    QSettings settings(QSettings::UserScope, "texturelab", "texturelab");
    settings.remove("updateChannel");
    settings.remove("updateKnownVersion");
}

void TestUpdateEndpoint::endpointIsAWellFormedAbsoluteUrl()
{
    // The request path is appended directly, so the base has to be absolute and
    // free of a trailing slash — "…:3333/" + "/api/…" is a double-slashed path
    // that some routers answer with a 404.
    const QString base = UpdateChecker::apiBase();

    QVERIFY(!base.isEmpty());
    QVERIFY(base.startsWith(QStringLiteral("http")));
    QVERIFY(!base.endsWith(QLatin1Char('/')));

    const QUrl url(base + QStringLiteral("/api/releases/latest"));
    QVERIFY(url.isValid());
    QCOMPARE(url.path(), QStringLiteral("/api/releases/latest"));
}

void TestUpdateEndpoint::endpointIgnoresTheEnvironment()
{
    // The endpoint is hardcoded. It used to be overridable, and this asserts
    // the override is really gone rather than quietly still honoured — a stale
    // variable in someone's shell would otherwise redirect update checks.
    const QString before = UpdateChecker::apiBase();

    qputenv("TEXTURELAB_API_BASE", "http://example.invalid:1234");
    QCOMPARE(UpdateChecker::apiBase(), before);
}

void TestUpdateEndpoint::channelFollowsOwnVersion()
{
    QCoreApplication::setApplicationVersion(QStringLiteral("0.4.0-beta+abc1234"));
    QCOMPARE(UpdateChecker::channel(), QStringLiteral("beta"));

    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0+abc1234"));
    QCOMPARE(UpdateChecker::channel(), QStringLiteral("stable"));
}

void TestUpdateEndpoint::channelSettingWinsOverVersion()
{
    QCoreApplication::setApplicationVersion(QStringLiteral("0.4.0-beta"));
    QSettings(QSettings::UserScope, "texturelab", "texturelab")
        .setValue(QStringLiteral("updateChannel"), QStringLiteral("stable"));

    QCOMPARE(UpdateChecker::channel(), QStringLiteral("stable"));
}

void TestUpdateEndpoint::nonsenseChannelSettingIsIgnored()
{
    QCoreApplication::setApplicationVersion(QStringLiteral("0.4.0-beta"));
    QSettings(QSettings::UserScope, "texturelab", "texturelab")
        .setValue(QStringLiteral("updateChannel"), QStringLiteral("nightly"));

    // The API only accepts stable|beta; anything else would come back 422.
    QCOMPARE(UpdateChecker::channel(), QStringLiteral("beta"));
}

void TestUpdateEndpoint::remembersAKnownUpdateAcrossRestarts()
{
    // The launcher shows the notice from this, not from a fresh request — the
    // network check is throttled to once every few hours, and without a
    // remembered answer the notice would vanish in between.
    QCoreApplication::setApplicationVersion(QStringLiteral("0.4.0-beta+abc1234"));

    QSettings settings(QSettings::UserScope, "texturelab", "texturelab");
    settings.setValue(QStringLiteral("updateKnownVersion"), QStringLiteral("1.3.0-beta"));

    QCOMPARE(UpdateChecker::knownUpdateVersion(), QStringLiteral("1.3.0-beta"));
}

void TestUpdateEndpoint::forgetsAKnownUpdateOnceTheBuildCatchesUp()
{
    QSettings settings(QSettings::UserScope, "texturelab", "texturelab");
    settings.setValue(QStringLiteral("updateKnownVersion"), QStringLiteral("1.3.0-beta"));

    // Running exactly the remembered version: nothing left to announce.
    QCoreApplication::setApplicationVersion(QStringLiteral("1.3.0-beta+deadbee"));
    QVERIFY(UpdateChecker::knownUpdateVersion().isEmpty());

    // And past it, which is what a beta tester hits after installing.
    QCoreApplication::setApplicationVersion(QStringLiteral("1.4.0"));
    QVERIFY(UpdateChecker::knownUpdateVersion().isEmpty());

    settings.remove(QStringLiteral("updateKnownVersion"));
    QVERIFY(UpdateChecker::knownUpdateVersion().isEmpty());
}

QTEST_GUILESS_MAIN(TestUpdateEndpoint)
#include "tst_updateendpoint.moc"
