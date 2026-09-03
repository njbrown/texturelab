#include "jsonutils.h"

#include <QtTest>

using namespace jsonutils;

class TestJsonUtils : public QObject {
    Q_OBJECT

private slots:
    void readsPlainNumbers();
    void readsNumbersStoredAsStrings();
    void roundsFloatsStoredForInts();
    void readsBoolsInEveryFormThatWasWritten();
    void readsNumbersStoredAsStringsForBools();
    void stringifiesNumbers();
    void fallsBackWhenMissingOrJunk();
};

void TestJsonUtils::readsPlainNumbers()
{
    QCOMPARE(getDouble(QJsonValue(1.5)), 1.5);
    QCOMPARE(getFloat(QJsonValue(1.5)), 1.5f);
    QCOMPARE(getInt(QJsonValue(3)), 3);
    QCOMPARE(getLong(QJsonValue(2147483648.0)), 2147483648L);
}

void TestJsonUtils::readsNumbersStoredAsStrings()
{
    // Old files wrote prop values through JS string conversion.
    QCOMPARE(getDouble(QJsonValue(QStringLiteral("1.5"))), 1.5);
    QCOMPARE(getDouble(QJsonValue(QStringLiteral(" -0.25 "))), -0.25);
    QCOMPARE(getInt(QJsonValue(QStringLiteral("7"))), 7);
    QCOMPARE(getFloat(QJsonValue(QStringLiteral("1e2"))), 100.0f);
}

void TestJsonUtils::roundsFloatsStoredForInts()
{
    // An int prop saved as 3.0 (or 2.7 after a slider drag) must not truncate
    // to something a step below what the user set.
    QCOMPARE(getInt(QJsonValue(3.0)), 3);
    QCOMPARE(getInt(QJsonValue(2.7)), 3);
    QCOMPARE(getInt(QJsonValue(QStringLiteral("2.7"))), 3);
    QCOMPARE(getInt(QJsonValue(-2.7)), -3);
}

void TestJsonUtils::readsBoolsInEveryFormThatWasWritten()
{
    QCOMPARE(getBool(QJsonValue(true)), true);
    // BoolProp::toJson still writes the string form.
    QCOMPARE(getBool(QJsonValue(QStringLiteral("true"))), true);
    QCOMPARE(getBool(QJsonValue(QStringLiteral("TRUE"))), true);
    QCOMPARE(getBool(QJsonValue(QStringLiteral("false"))), false);
    QCOMPARE(getBool(QJsonValue(1.0)), true);
    QCOMPARE(getBool(QJsonValue(0.0)), false);
}

void TestJsonUtils::readsNumbersStoredAsStringsForBools()
{
    QCOMPARE(getBool(QJsonValue(QStringLiteral("1"))), true);
    QCOMPARE(getBool(QJsonValue(QStringLiteral("0"))), false);
}

void TestJsonUtils::stringifiesNumbers()
{
    QCOMPARE(getString(QJsonValue(3.0)), QStringLiteral("3"));
    QCOMPARE(getString(QJsonValue(QStringLiteral("abc"))), QStringLiteral("abc"));
}

void TestJsonUtils::fallsBackWhenMissingOrJunk()
{
    QJsonObject obj;
    QCOMPARE(getDouble(obj["missing"], 300.0), 300.0);
    QCOMPARE(getFloat(QJsonValue(QStringLiteral("not a number")), 2.5f), 2.5f);
    QCOMPARE(getBool(QJsonValue(QJsonValue::Null), true), true);
    QCOMPARE(getInt(QJsonValue(QJsonArray()), 9), 9);
}

QTEST_APPLESS_MAIN(TestJsonUtils)
#include "tst_jsonutils.moc"
