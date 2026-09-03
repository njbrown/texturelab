#include "libversion.h"
#include "library.h"

LibVersion currentLibVersion() { return kCurrentLibVersion; }

LibVersion libVersionFromString(const QString& str)
{
    auto s = str.trimmed().toLower();
    if (s == "v1")
        return LibVersion::V1;
    if (s == "v2")
        return LibVersion::V2;
    if (s == "v3")
        return LibVersion::V3;

    return currentLibVersion();
}

QString libVersionToString(LibVersion version)
{
    switch (version) {
    case LibVersion::V1: return "v1";
    case LibVersion::V2: return "v2";
    case LibVersion::V3: return "v3";
    }
    return "v3";
}

bool hasNextLibVersion(LibVersion version)
{
    return version != currentLibVersion();
}

LibVersion nextLibVersion(LibVersion version)
{
    return static_cast<LibVersion>(static_cast<int>(version) + 1);
}

Library* createLibraryForVersion(LibVersion version)
{
    switch (version) {
    case LibVersion::V1:
    case LibVersion::V2:
        return createLibraryV2();
    case LibVersion::V3:
        return createLibraryV3();
    }
    return createLibraryV3();
}
