#include "versioncompare.h"

#include <QtTest>

using namespace appversion;

namespace {

Build make(const QString& version, int number, const QString& channel = QStringLiteral("beta"),
           const QString& commit = QStringLiteral("abc1234"))
{
    Build build;
    build.version = version;
    build.channel = channel;
    build.number = number;
    build.commit = commit;
    return build;
}

} // namespace

class TestVersionCompare : public QObject {
    Q_OBJECT

private slots:
    void identifierJoinsTheParts();
    void validityRejectsWhatCantBeOrdered();

    void comparesNumericParts();
    void buildNumberOrdersBuildsOfOneVersion();
    void versionWinsOverBuildNumber();
    void commitAndChannelPlayNoPartInOrder();

    void thisBuildIsNotNewerThanItself();
    void newerStableSupersedesOlderBeta();
    void invalidInputNeverClaimsAnUpdate();
};

void TestVersionCompare::identifierJoinsTheParts()
{
    QCOMPARE(make(QStringLiteral("0.4.0"), 963, QStringLiteral("beta"), QStringLiteral("039faa9"))
                 .identifier(),
             QStringLiteral("0.4.0-beta-039faa9"));
    QCOMPARE(make(QStringLiteral("1.0.0"), 1200, QStringLiteral("stable"), QStringLiteral("5229137"))
                 .identifier(),
             QStringLiteral("1.0.0-stable-5229137"));
}

void TestVersionCompare::validityRejectsWhatCantBeOrdered()
{
    QVERIFY(make(QStringLiteral("0.4.0"), 1).isValid());

    // The channel is its own field now; a suffix in the version means the
    // server (or a caller) is still speaking the old scheme.
    QVERIFY(!make(QStringLiteral("0.4.0-beta"), 963).isValid());
    QVERIFY(!make(QStringLiteral("0.4"), 963).isValid());
    QVERIFY(!make(QStringLiteral("v0.4.0"), 963).isValid());
    QVERIFY(!make(QString(), 963).isValid());

    // No git at configure time leaves the number at 0.
    QVERIFY(!make(QStringLiteral("0.4.0"), 0).isValid());
    QVERIFY(!make(QStringLiteral("0.4.0"), -1).isValid());

    QVERIFY(!make(QStringLiteral("0.4.0"), 963, QStringLiteral("nightly")).isValid());
    QVERIFY(!make(QStringLiteral("0.4.0"), 963, QString()).isValid());
}

void TestVersionCompare::comparesNumericParts()
{
    QVERIFY(isNewer(make(QStringLiteral("1.0.1"), 1), make(QStringLiteral("1.0.0"), 1)));
    QVERIFY(isNewer(make(QStringLiteral("1.1.0"), 1), make(QStringLiteral("1.0.9"), 1)));
    QVERIFY(isNewer(make(QStringLiteral("2.0.0"), 1), make(QStringLiteral("1.9.9"), 1)));
    QVERIFY(!isNewer(make(QStringLiteral("1.0.0"), 1), make(QStringLiteral("1.0.1"), 1)));

    // Not a string comparison: "0.10.0" is newer than "0.9.0" even though it
    // sorts lower lexically. This is the classic way this goes wrong.
    QVERIFY(isNewer(make(QStringLiteral("0.10.0"), 1), make(QStringLiteral("0.9.0"), 1)));
    QVERIFY(isNewer(make(QStringLiteral("0.4.10"), 1), make(QStringLiteral("0.4.9"), 1)));
}

void TestVersionCompare::buildNumberOrdersBuildsOfOneVersion()
{
    // Every beta of a cycle is "0.4.0"; only the commit count tells them apart.
    QVERIFY(isNewer(make(QStringLiteral("0.4.0"), 1015), make(QStringLiteral("0.4.0"), 963)));
    QVERIFY(!isNewer(make(QStringLiteral("0.4.0"), 963), make(QStringLiteral("0.4.0"), 1015)));
}

void TestVersionCompare::versionWinsOverBuildNumber()
{
    // A stable hotfix cut from a release branch can have fewer commits than
    // master's latest beta; it is still the newer version.
    QVERIFY(isNewer(make(QStringLiteral("0.4.1"), 990, QStringLiteral("stable")),
                    make(QStringLiteral("0.4.0"), 1015)));
    QVERIFY(!isNewer(make(QStringLiteral("0.4.0"), 2000), make(QStringLiteral("0.5.0"), 10)));
}

void TestVersionCompare::commitAndChannelPlayNoPartInOrder()
{
    const Build beta = make(QStringLiteral("0.4.0"), 963, QStringLiteral("beta"),
                            QStringLiteral("fffffff"));
    const Build stable = make(QStringLiteral("0.4.0"), 963, QStringLiteral("stable"),
                              QStringLiteral("0000000"));

    QCOMPARE(compare(beta, stable), 0);
    QVERIFY(!isNewer(beta, stable));
    QVERIFY(!isNewer(stable, beta));
}

void TestVersionCompare::thisBuildIsNotNewerThanItself()
{
    // The check that must never fire: the server publishing exactly the build
    // the user is running.
    const Build self = make(QStringLiteral("0.4.0"), 963, QStringLiteral("beta"),
                            QStringLiteral("039faa9"));
    QVERIFY(!isNewer(self, self));
}

void TestVersionCompare::newerStableSupersedesOlderBeta()
{
    // The server answers a beta build with a stable when that is newer; the
    // client has to agree it is newer for the notice to show.
    QVERIFY(isNewer(make(QStringLiteral("0.4.0"), 1100, QStringLiteral("stable")),
                    make(QStringLiteral("0.4.0"), 963, QStringLiteral("beta"))));

    // ...and an older stable doesn't pull a beta user backwards.
    QVERIFY(!isNewer(make(QStringLiteral("0.3.3"), 900, QStringLiteral("stable")),
                     make(QStringLiteral("0.4.0"), 963, QStringLiteral("beta"))));
}

void TestVersionCompare::invalidInputNeverClaimsAnUpdate()
{
    const Build current = make(QStringLiteral("0.4.0"), 963);

    QVERIFY(!isNewer(make(QStringLiteral("garbage"), 99999), current));
    QVERIFY(!isNewer(make(QStringLiteral("9.9.9-beta"), 99999), current));
    QVERIFY(!isNewer(make(QStringLiteral("9.9.9"), 0), current));

    // And a running build that can't describe itself is told nothing.
    QVERIFY(!isNewer(make(QStringLiteral("9.9.9"), 99999), make(QStringLiteral("0.4.0"), 0)));
}

QTEST_GUILESS_MAIN(TestVersionCompare)
#include "tst_versioncompare.moc"
