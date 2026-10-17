#pragma once

#include <QString>

// What a build is, and whether one build is newer than another.
//
// Kept apart from UpdateChecker, and free of any dependency beyond QtCore, so
// the ordering rules can be tested without a network stack. Getting this wrong
// is quiet and bad in both directions: nagging users who are already current,
// or never telling anyone an update exists.
namespace appversion {

// One build, as four separate facts. The site's API returns the same four, so
// nothing ever has to pull them back out of a combined string.
struct Build {
    // Numeric "major.minor.patch" only. The channel is not a pre-release tag:
    // a beta and a stable can share a version, and every beta of a cycle does.
    QString version;

    // "stable" or "beta".
    QString channel;

    // Git commit count. What orders two builds of the same version.
    int number = 0;

    // Short git hash. Identifies a build; never compared for order.
    QString commit;

    // "0.4.0-beta-039faa9": the name shown in-app, in artifact names and on
    // the site. A label only — never parsed back, never compared.
    QString identifier() const;

    // A well-formed numeric version, a known channel and a positive number.
    // Anything less from the server means "no update", never a guess.
    bool isValid() const;
};

bool isKnownChannel(const QString& channel);

// -1 if a < b, 0 if equal, 1 if a > b, by (major, minor, patch, number).
//
// Version first, so a stable hotfix cut from a release branch (whose commit
// count can be lower than master's) still orders by what it is. The channel
// and commit play no part: the same (version, number) is the same point in
// history whichever channel it was published on.
int compare(const Build& a, const Build& b);

// True when `candidate` is strictly newer than `current`.
bool isNewer(const Build& candidate, const Build& current);

} // namespace appversion
