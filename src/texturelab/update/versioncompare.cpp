#include "versioncompare.h"

#include <QRegularExpression>
#include <QStringList>

namespace appversion {

namespace {

struct Numeric {
    int major = 0;
    int minor = 0;
    int patch = 0;
};

// Only called on versions isValid() accepted, so every part is a number.
Numeric parse(const QString& version)
{
    const QStringList parts = version.split(QLatin1Char('.'));

    Numeric out;
    out.major = parts.value(0).toInt();
    out.minor = parts.value(1).toInt();
    out.patch = parts.value(2).toInt();
    return out;
}

int order(int a, int b)
{
    return a < b ? -1 : (a > b ? 1 : 0);
}

} // namespace

QString Build::identifier() const
{
    return QStringLiteral("%1-%2-%3").arg(version, channel, commit);
}

bool Build::isValid() const
{
    static const QRegularExpression numeric(QStringLiteral("^\\d{1,9}\\.\\d{1,9}\\.\\d{1,9}$"));

    return numeric.match(version).hasMatch() && isKnownChannel(channel) && number > 0;
}

bool isKnownChannel(const QString& channel)
{
    return channel == QLatin1String("stable") || channel == QLatin1String("beta");
}

int compare(const Build& a, const Build& b)
{
    const Numeric left = parse(a.version);
    const Numeric right = parse(b.version);

    if (int c = order(left.major, right.major))
        return c;
    if (int c = order(left.minor, right.minor))
        return c;
    if (int c = order(left.patch, right.patch))
        return c;

    return order(a.number, b.number);
}

bool isNewer(const Build& candidate, const Build& current)
{
    // An unparseable side can't be ordered, and "maybe newer" is not an update.
    if (!candidate.isValid() || !current.isValid())
        return false;

    return compare(candidate, current) > 0;
}

} // namespace appversion
