#pragma once

#include "libversion.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QVector>

// One node's identity/shape change for a single version step (from -> next).
struct NodeTypeMigration {
    QString oldTypeName;
    QString newTypeName;

    // Rare: a property key that was renamed on an otherwise-equivalent
    // node. Old key -> new key. Empty for every step table that exists
    // today (see libraryversionmigrator.cpp).
    QMap<QString, QString> propertyKeyRenames;
};

// Returns the migration table to apply when stepping from `from` to
// nextLibVersion(from). An empty vector means that step has no typeName
// changes at all (e.g. v1 -> v2 today).
QVector<NodeTypeMigration> migrationStepTable(LibVersion from);

// Migrates a project's raw JSON from whatever library version it declares
// up to a target version (defaults to the current one), walking one
// version step at a time. Pure data transform: only QJsonObject /
// QJsonArray / QString are touched. No Library, TextureNode, Prop, or
// OpenGL/GPU resource is created anywhere in this class.
class LibraryVersionMigrator {
public:
    explicit LibraryVersionMigrator(QJsonObject projectJson);

    LibVersion sourceVersion() const { return _source; }
    LibVersion targetVersion() const { return _target; }
    void setTargetVersion(LibVersion version) { _target = version; }

    bool needsMigration() const;

    // The chain of versions that migrate() will step into, e.g. [V2, V3]
    // when migrating a V1 file up to V3. Empty if needsMigration() is
    // false. Intended for building a human-readable "v1 -> v2 -> v3"
    // message.
    QList<LibVersion> versionsCrossed() const;

    // Returns the migrated project JSON. Does not mutate the JSON passed
    // to the constructor. Idempotent. Sets "libraryVersion" on the result
    // to libVersionToString(targetVersion()).
    QJsonObject migrate() const;

private:
    QJsonObject _json;
    LibVersion _source;
    LibVersion _target;

    static QJsonArray applyStep(const QJsonArray& nodes,
                                const QVector<NodeTypeMigration>& table);
    static bool looksLegacy(const QJsonObject& json);
};
