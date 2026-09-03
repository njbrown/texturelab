#pragma once

#include <QString>

class Library;

// Contiguous, ordered library versions. Adding a new version means: add the
// enum value here, move kCurrentLibVersion, extend createLibraryForVersion(),
// and add a migration step table in libraryversionmigrator.cpp.
enum class LibVersion { V1 = 0, V2 = 1, V3 = 2 };

constexpr LibVersion kCurrentLibVersion = LibVersion::V3;

LibVersion currentLibVersion();

// Parses "v1"/"v2"/"v3" (case-insensitive). Unrecognized/empty strings fall
// back to currentLibVersion().
LibVersion libVersionFromString(const QString& str);
QString libVersionToString(LibVersion version);

bool hasNextLibVersion(LibVersion version);
LibVersion nextLibVersion(LibVersion version);

// Returns the node library that should be used to load/edit a project at
// the given version. V1 and V2 currently share createLibraryV2() since the
// v1->v2 step never removed or renamed anything.
Library* createLibraryForVersion(LibVersion version);
