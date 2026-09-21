#include "versioncompare.h"

#include <QtTest>

using namespace appversion;

class TestVersionCompare : public QObject {
    Q_OBJECT

private slots:
    void normalizeStripsBuildMetadataAndPrefix();

    void comparesNumericParts();
    void treatsMissingPartsAsZero();
    void preReleaseSortsBelowRelease();
    void comparesPreReleaseIdentifiers();

    void thisBuildIsNotNewerThanItself();
    void betaBuildSeesMatchingStableRelease();
    void malformedInputNeverClaimsAnUpdate();
};

void TestVersionCompare::normalizeStripsBuildMetadataAndPrefix()
{
    // The app's own version is "0.4.0-beta+<githash>"; the API publishes
    // "0.4.0-beta". Without stripping the metadata every check would either
    // compare unequal strings or mis-parse the hash as a version part.
    QCOMPARE(normalize(QStringLiteral("0.4.0-beta+a1b2c3d")), QStringLiteral("0.4.0-beta"));
    QCOMPARE(normalize(QStringLiteral("v1.2.3")), QStringLiteral("1.2.3"));
    QCOMPARE(normalize(QStringLiteral("  1.2.3  ")), QStringLiteral("1.2.3"));
}

void TestVersionCompare::comparesNumericParts()
{
    QCOMPARE(compare(QStringLiteral("1.0.0"), QStringLiteral("1.0.0")), 0);
    QVERIFY(isNewer(QStringLiteral("1.0.1"), QStringLiteral("1.0.0")));
    QVERIFY(isNewer(QStringLiteral("1.1.0"), QStringLiteral("1.0.9")));
    QVERIFY(isNewer(QStringLiteral("2.0.0"), QStringLiteral("1.9.9")));
    QVERIFY(!isNewer(QStringLiteral("1.0.0"), QStringLiteral("1.0.1")));

    // Not a string comparison: "0.10.0" is newer than "0.9.0" even though it
    // sorts lower lexically. This is the classic way this goes wrong.
    QVERIFY(isNewer(QStringLiteral("0.10.0"), QStringLiteral("0.9.0")));
    QVERIFY(isNewer(QStringLiteral("0.4.10"), QStringLiteral("0.4.9")));
}

void TestVersionCompare::treatsMissingPartsAsZero()
{
    QCOMPARE(compare(QStringLiteral("1.2"), QStringLiteral("1.2.0")), 0);
    QCOMPARE(compare(QStringLiteral("1"), QStringLiteral("1.0.0")), 0);
    QVERIFY(isNewer(QStringLiteral("1.2.1"), QStringLiteral("1.2")));
}

void TestVersionCompare::preReleaseSortsBelowRelease()
{
    // Semver's rule, and the one that matters most here: shipping 0.4.0 final
    // must register as an update for someone on 0.4.0-beta.
    QVERIFY(isNewer(QStringLiteral("0.4.0"), QStringLiteral("0.4.0-beta")));
    QVERIFY(!isNewer(QStringLiteral("0.4.0-beta"), QStringLiteral("0.4.0")));
    QCOMPARE(compare(QStringLiteral("0.4.0-beta"), QStringLiteral("0.4.0-beta")), 0);
}

void TestVersionCompare::comparesPreReleaseIdentifiers()
{
    QVERIFY(isNewer(QStringLiteral("1.0.0-beta.2"), QStringLiteral("1.0.0-beta.1")));
    QVERIFY(isNewer(QStringLiteral("1.0.0-beta.10"), QStringLiteral("1.0.0-beta.9")));
    QVERIFY(isNewer(QStringLiteral("1.0.0-rc"), QStringLiteral("1.0.0-beta")));

    // A longer run of otherwise-equal identifiers is the higher version.
    QVERIFY(isNewer(QStringLiteral("1.0.0-beta.1"), QStringLiteral("1.0.0-beta")));

    // Numeric identifiers rank below alphanumeric ones.
    QVERIFY(isNewer(QStringLiteral("1.0.0-alpha"), QStringLiteral("1.0.0-1")));
}

void TestVersionCompare::thisBuildIsNotNewerThanItself()
{
    // The exact shape the app compares at runtime: its own version string, with
    // the build hash attached, against what the API would publish for it.
    const QString running = QStringLiteral("0.4.0-beta+e759268");
    QVERIFY(!isNewer(QStringLiteral("0.4.0-beta"), running));
    QCOMPARE(compare(QStringLiteral("0.4.0-beta"), running), 0);
}

void TestVersionCompare::betaBuildSeesMatchingStableRelease()
{
    const QString running = QStringLiteral("0.4.0-beta+e759268");
    QVERIFY(isNewer(QStringLiteral("0.4.0"), running));
    QVERIFY(isNewer(QStringLiteral("0.5.0-beta"), running));
}

void TestVersionCompare::malformedInputNeverClaimsAnUpdate()
{
    // A broken response should mean "no update", never a false prompt and never
    // a crash.
    const QString running = QStringLiteral("0.4.0-beta+e759268");
    QVERIFY(!isNewer(QString(), running));
    QVERIFY(!isNewer(QStringLiteral("not-a-version"), running));
    QVERIFY(!isNewer(QStringLiteral("...."), running));
    QVERIFY(!isNewer(QStringLiteral("0.0.0"), running));
}

QTEST_GUILESS_MAIN(TestVersionCompare)
#include "tst_versioncompare.moc"
