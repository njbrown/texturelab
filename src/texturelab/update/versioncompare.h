#pragma once

#include <QString>

// Semver comparison for "is the release on the site newer than this build?".
//
// Kept apart from UpdateChecker, and free of any dependency beyond QtCore, so
// the ordering rules can be tested without a network stack. Getting this wrong
// is quiet and bad in both directions: nagging users who are already current,
// or never telling anyone an update exists.
namespace appversion {

// -1 if a < b, 0 if equal, 1 if a > b.
//
// Follows semver precedence, including the rule people get wrong: a release
// with a pre-release tag sorts *below* the same numbers without one, so
// 0.4.0-beta < 0.4.0. Build metadata after '+' is ignored entirely, which is
// what lets the app compare its own "0.4.0-beta+a1b2c3d" against the plain
// "0.4.0-beta" the API publishes.
//
// Missing numeric parts are zero, so "1.2" == "1.2.0". Non-numeric junk in a
// numeric field compares as 0 rather than throwing — a malformed version from
// the server should mean "no update", never a crash.
int compare(const QString& a, const QString& b);

// True when `candidate` is strictly newer than `current`.
bool isNewer(const QString& candidate, const QString& current);

// Strips build metadata and any leading 'v' — "v1.2.3+abc" becomes "1.2.3".
QString normalize(const QString& version);

} // namespace appversion
