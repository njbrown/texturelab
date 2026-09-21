#include "versioncompare.h"

#include <QStringList>

namespace appversion {

namespace {

struct Parsed {
    int major = 0;
    int minor = 0;
    int patch = 0;
    QString preRelease;
};

int toInt(const QString& text)
{
    bool ok = false;
    const int value = text.toInt(&ok);
    return ok && value >= 0 ? value : 0;
}

Parsed parse(const QString& version)
{
    Parsed out;

    QString core = normalize(version);

    const int dash = core.indexOf(QLatin1Char('-'));
    if (dash >= 0) {
        out.preRelease = core.mid(dash + 1);
        core = core.left(dash);
    }

    const QStringList parts = core.split(QLatin1Char('.'));
    if (parts.size() > 0)
        out.major = toInt(parts[0]);
    if (parts.size() > 1)
        out.minor = toInt(parts[1]);
    if (parts.size() > 2)
        out.patch = toInt(parts[2]);

    return out;
}

// Semver identifier comparison: numeric identifiers compare numerically and
// always sort below alphanumeric ones; a shorter run of equal identifiers sorts
// lower. So beta < beta.2 < rc.
int comparePreRelease(const QString& a, const QString& b)
{
    if (a == b)
        return 0;

    // Neither having a pre-release is handled by the caller; here, exactly one
    // being empty means that one is the full release and therefore greater.
    if (a.isEmpty())
        return 1;
    if (b.isEmpty())
        return -1;

    const QStringList left = a.split(QLatin1Char('.'));
    const QStringList right = b.split(QLatin1Char('.'));

    for (int i = 0; i < qMax(left.size(), right.size()); ++i) {
        if (i >= left.size())
            return -1;
        if (i >= right.size())
            return 1;

        const QString& l = left[i];
        const QString& r = right[i];

        bool lNumeric = false;
        bool rNumeric = false;
        const int lValue = l.toInt(&lNumeric);
        const int rValue = r.toInt(&rNumeric);

        if (lNumeric && rNumeric) {
            if (lValue != rValue)
                return lValue < rValue ? -1 : 1;
            continue;
        }
        if (lNumeric != rNumeric)
            return lNumeric ? -1 : 1;

        const int textual = QString::compare(l, r);
        if (textual != 0)
            return textual < 0 ? -1 : 1;
    }

    return 0;
}

} // namespace

QString normalize(const QString& version)
{
    QString out = version.trimmed();

    if (out.startsWith(QLatin1Char('v')) || out.startsWith(QLatin1Char('V')))
        out = out.mid(1);

    const int plus = out.indexOf(QLatin1Char('+'));
    if (plus >= 0)
        out = out.left(plus);

    return out;
}

int compare(const QString& a, const QString& b)
{
    const Parsed left = parse(a);
    const Parsed right = parse(b);

    if (left.major != right.major)
        return left.major < right.major ? -1 : 1;
    if (left.minor != right.minor)
        return left.minor < right.minor ? -1 : 1;
    if (left.patch != right.patch)
        return left.patch < right.patch ? -1 : 1;

    if (left.preRelease.isEmpty() && right.preRelease.isEmpty())
        return 0;

    return comparePreRelease(left.preRelease, right.preRelease);
}

bool isNewer(const QString& candidate, const QString& current)
{
    return compare(candidate, current) > 0;
}

} // namespace appversion
