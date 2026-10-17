#include "updatechecker.h"

#include "telemetry.h"

#include <QCoreApplication>
#include <QJsonObject>
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

    void channelIsTheBuildsOwn();
    void channelIgnoresStoredSetting();

    void offersANewerBuildOnTheSameChannel();
    void betaIsOfferedANewerStable();
    void stableIsNeverOfferedABeta();
    void doesNotOfferTheRunningBuild();
    void rejectsAResponseInTheOldFormat();
    void downloadLinkStaysOnTheSite();
};

namespace {

appversion::Build makeBuild(const QString& channel, int number,
                            const QString& version = QStringLiteral("0.4.0"))
{
    appversion::Build build;
    build.version = version;
    build.channel = channel;
    build.number = number;
    build.commit = QStringLiteral("abc1234");
    return build;
}

// The shape /api/releases/latest returns under "data".
QJsonObject release(const QString& channel, int number,
                    const QString& version = QStringLiteral("0.4.0"),
                    const QString& commit = QStringLiteral("def5678"))
{
    QJsonObject data;
    data[QStringLiteral("identifier")] = QStringLiteral("%1-%2-%3").arg(version, channel, commit);
    data[QStringLiteral("version")] = version;
    data[QStringLiteral("channel")] = channel;
    data[QStringLiteral("build")] = number;
    data[QStringLiteral("commit")] = commit;
    data[QStringLiteral("title")] = QStringLiteral("TextureLab %1").arg(version);
    data[QStringLiteral("pageUrl")] = UpdateChecker::apiBase() + QStringLiteral("/download/") +
                                      data[QStringLiteral("identifier")].toString();
    return data;
}

} // namespace

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

void TestUpdateEndpoint::channelIsTheBuildsOwn()
{
    UpdateChecker::setCurrentBuild(makeBuild(QStringLiteral("beta"), 963));
    QCOMPARE(UpdateChecker::channel(), QStringLiteral("beta"));

    UpdateChecker::setCurrentBuild(makeBuild(QStringLiteral("stable"), 963));
    QCOMPARE(UpdateChecker::channel(), QStringLiteral("stable"));
}

void TestUpdateEndpoint::channelIgnoresStoredSetting()
{
    // The channel used to be a setting. A value left over in someone's config
    // must not move a beta build onto the stable channel, or vice versa.
    QSettings settings(QSettings::UserScope, "texturelab", "texturelab");
    settings.setValue(QStringLiteral("updateChannel"), QStringLiteral("stable"));

    UpdateChecker::setCurrentBuild(makeBuild(QStringLiteral("beta"), 963));
    QCOMPARE(UpdateChecker::channel(), QStringLiteral("beta"));

    settings.setValue(QStringLiteral("updateChannel"), QStringLiteral("beta"));
    UpdateChecker::setCurrentBuild(makeBuild(QStringLiteral("stable"), 963));
    QCOMPARE(UpdateChecker::channel(), QStringLiteral("stable"));
}

void TestUpdateEndpoint::offersANewerBuildOnTheSameChannel()
{
    const auto offer = UpdateChecker::evaluate(release(QStringLiteral("beta"), 1015),
                                               makeBuild(QStringLiteral("beta"), 963));
    QVERIFY(offer.found);
    QCOMPARE(offer.identifier, QStringLiteral("0.4.0-beta-def5678"));
    QVERIFY(offer.error.isEmpty());

    // An older build on the channel (running something built from source
    // ahead of the published beta) is not an update.
    QVERIFY(!UpdateChecker::evaluate(release(QStringLiteral("beta"), 900),
                                     makeBuild(QStringLiteral("beta"), 963))
                 .found);
}

void TestUpdateEndpoint::betaIsOfferedANewerStable()
{
    // The server answers ?channel=beta with a stable when it is newer.
    const auto offer = UpdateChecker::evaluate(release(QStringLiteral("stable"), 1100),
                                               makeBuild(QStringLiteral("beta"), 963));
    QVERIFY(offer.found);
    QCOMPARE(offer.identifier, QStringLiteral("0.4.0-stable-def5678"));
}

void TestUpdateEndpoint::stableIsNeverOfferedABeta()
{
    // Even if the server got it wrong and sent one.
    const auto offer = UpdateChecker::evaluate(release(QStringLiteral("beta"), 5000,
                                                       QStringLiteral("9.0.0")),
                                               makeBuild(QStringLiteral("stable"), 963));
    QVERIFY(!offer.found);
    QVERIFY(offer.error.isEmpty());
}

void TestUpdateEndpoint::doesNotOfferTheRunningBuild()
{
    // This week's bug, in the new scheme: the published release is the build
    // being run, and nothing should be shown.
    const auto offer = UpdateChecker::evaluate(release(QStringLiteral("beta"), 963),
                                               makeBuild(QStringLiteral("beta"), 963));
    QVERIFY(!offer.found);
    QVERIFY(offer.error.isEmpty());
}

void TestUpdateEndpoint::rejectsAResponseInTheOldFormat()
{
    // The pre-redesign API: a combined version string, no build number. It
    // must read as "unusable", not be guessed at.
    QJsonObject old;
    old[QStringLiteral("version")] = QStringLiteral("0.4.0-beta");
    old[QStringLiteral("channel")] = QStringLiteral("beta");

    const auto offer = UpdateChecker::evaluate(old, makeBuild(QStringLiteral("beta"), 963));
    QVERIFY(!offer.found);
    QVERIFY(!offer.error.isEmpty());
}

void TestUpdateEndpoint::downloadLinkStaysOnTheSite()
{
    const appversion::Build current = makeBuild(QStringLiteral("beta"), 963);

    QJsonObject data = release(QStringLiteral("beta"), 1015);
    QCOMPARE(UpdateChecker::evaluate(data, current).downloadUrl,
             UpdateChecker::apiBase() + QStringLiteral("/download/0.4.0-beta-def5678"));

    data[QStringLiteral("pageUrl")] = QStringLiteral("https://example.invalid/download");
    QCOMPARE(UpdateChecker::evaluate(data, current).downloadUrl,
             UpdateChecker::apiBase() + QStringLiteral("/download"));

    data.remove(QStringLiteral("pageUrl"));
    QCOMPARE(UpdateChecker::evaluate(data, current).downloadUrl,
             UpdateChecker::apiBase() + QStringLiteral("/download"));
}

QTEST_GUILESS_MAIN(TestUpdateEndpoint)
#include "tst_updateendpoint.moc"
