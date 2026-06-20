#include "libraryversionmigrator.h"

namespace {

// v2 -> v3. The only non-empty step table that exists today. Every other
// v2 typeName resolves to the same class in v3 and needs no entry here —
// only the typeNames that were actually removed, renamed, or silently
// swapped for a different implementation are listed.
//
// No propertyKeyRenames are needed for any of these: the existing
// node-load loop (Project::loadTextureFromJson) already applies JSON
// properties by key and skips anything the target node doesn't have,
// which reproduces "copy matching props, drop the rest, default the new
// ones" without any extra code here.
const QVector<NodeTypeMigration>& v2ToV3Table()
{
    static const QVector<NodeTypeMigration> table = {
        {"floodfill", "floodfillv2", {}},
        {"floodfillsampler", "floodfillv2sampler", {}},
        {"floodfilltobbox", "floodfillv2tobbox", {}},
        {"floodfilltocolor", "floodfillv2tocolor", {}},
        {"floodfilltogradient", "floodfillv2togradient", {}},
        {"floodfilltorandomcolor", "floodfillv2torandomcolor", {}},
        {"floodfilltorandomintensity", "floodfillv2torandomintensity", {}},
        {"bevel", "bevelv2", {}},
        {"perlin3d", "perlinnoise3d", {}},
        {"blend", "blend", {}},       // same name, new class (BlendV3Node)
        {"cell", "cell", {}},         // same name, new class (CellV3Node)
        {"linecell", "linecell", {}}, // same name, new class (LineCellV3Node)
        {"solidcell", "solidcell", {}}, // same name, new class (SolidCellV3Node)
    };
    return table;
}

} // namespace

QVector<NodeTypeMigration> migrationStepTable(LibVersion from)
{
    switch (from) {
    case LibVersion::V1:
        return {}; // v1 -> v2 is purely additive, nothing to migrate
    case LibVersion::V2:
        return v2ToV3Table();
    case LibVersion::V3:
        return {}; // current version: no further step
    }
    return {};
}

LibraryVersionMigrator::LibraryVersionMigrator(QJsonObject projectJson)
    : _json(std::move(projectJson)), _target(currentLibVersion())
{
    auto versionStr = _json["libraryVersion"].toString();
    if (!versionStr.isEmpty())
        _source = libVersionFromString(versionStr);
    else
        _source = looksLegacy(_json) ? LibVersion::V2 : currentLibVersion();
}

bool LibraryVersionMigrator::needsMigration() const
{
    return _source != _target;
}

QList<LibVersion> LibraryVersionMigrator::versionsCrossed() const
{
    QList<LibVersion> chain;
    for (LibVersion v = _source; v != _target; v = nextLibVersion(v))
        chain.append(nextLibVersion(v));
    return chain;
}

QJsonObject LibraryVersionMigrator::migrate() const
{
    QJsonObject result = _json;
    QJsonArray nodes = result["nodes"].toArray();

    for (LibVersion v = _source; v != _target; v = nextLibVersion(v))
        nodes = applyStep(nodes, migrationStepTable(v));

    result["nodes"] = nodes;
    result["libraryVersion"] = libVersionToString(_target);
    return result;
}

QJsonArray LibraryVersionMigrator::applyStep(
    const QJsonArray& nodes, const QVector<NodeTypeMigration>& table)
{
    if (table.isEmpty())
        return nodes;

    QJsonArray result;
    for (const auto& item : nodes) {
        auto nodeObj = item.toObject();
        auto typeName = nodeObj["typeName"].toString();

        for (const auto& entry : table) {
            if (entry.oldTypeName != typeName)
                continue;

            nodeObj["typeName"] = entry.newTypeName;

            if (!entry.propertyKeyRenames.isEmpty()) {
                auto props = nodeObj["properties"].toObject();
                for (auto it = entry.propertyKeyRenames.begin();
                     it != entry.propertyKeyRenames.end(); ++it) {
                    if (props.contains(it.key())) {
                        props[it.value()] = props[it.key()];
                        props.remove(it.key());
                    }
                }
                nodeObj["properties"] = props;
            }
            break;
        }

        result.append(nodeObj);
    }
    return result;
}

bool LibraryVersionMigrator::looksLegacy(const QJsonObject& json)
{
    auto nodes = json["nodes"].toArray();
    for (const auto& item : nodes) {
        auto typeName = item.toObject()["typeName"].toString();
        for (const auto& entry : v2ToV3Table()) {
            // Entries where oldTypeName == newTypeName (blend, cell,
            // linecell, solidcell) only changed which class backs that
            // name — the name itself is just as valid in a current-version
            // file, so it can't be used as a legacy signal. Only count
            // typeNames that were actually removed/renamed.
            if (entry.oldTypeName == entry.newTypeName)
                continue;
            if (entry.oldTypeName == typeName)
                return true;
        }
    }
    return false;
}
