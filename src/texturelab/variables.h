#pragma once

// Texture variables: project-level values that drive node props.
//
// A variable owns a Prop (FloatProp/IntProp/BoolProp/EnumProp/ColorProp) that
// holds its value, bounds and name/displayName, plus the list of node props
// linked to it. The variable is the source of truth: TextureProject::
// applyVariable() writes its value into every linked prop.
//
// Serialized in the .texture file's "variables" array using the field names of
// the Electron 0.3 format (id, numeric type, property, linkedProperties) — see
// docs/texture-variables-plan.md.

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QSharedPointer>
#include <QString>
#include <QVariant>
#include <functional>

class Prop;
class TextureVariable;
typedef QSharedPointer<TextureVariable> TextureVariablePtr;

// Written to files as a string ("float", "gradient", ...). Files from the
// Electron 0.3 app store a number instead; 1-5 match its DesignerVariableType
// and are still read.
enum class VariableType : int {
    None = 0,
    Float = 1,
    Int = 2,
    Bool = 3,
    Enum = 4,
    Color = 5,
    Gradient = 6,
    Curve = 7,
    Image = 8,
    String = 9,
};

struct VariableLink {
    QString nodeId;
    QString propName;

    bool operator==(const VariableLink& o) const
    {
        return nodeId == o.nodeId && propName == o.propName;
    }
};

// One option an enum variable offers. `index` is into the linked props' own
// option list; `label` is what the variable shows for it.
struct EnumOption {
    int index = 0;
    QString label;

    bool operator==(const EnumOption& o) const
    {
        return index == o.index && label == o.label;
    }
};

class TextureVariable {
public:
    QString id;
    VariableType type = VariableType::None;
    // Owned. Same class as the props it links to. Holds the value, bounds
    // (Float/Int), option list (Enum), and the variable's identifier
    // (prop->name) and display name (prop->displayName).
    Prop* prop = nullptr;
    // Enum only. Empty = every option of the prop, with its original label.
    QList<EnumOption> options;
    QList<VariableLink> links;

    TextureVariable() = default;
    ~TextureVariable();
    TextureVariable(const TextureVariable&) = delete;
    TextureVariable& operator=(const TextureVariable&) = delete;

    QString name() const;
    QString displayName() const;

    // Exposable prop types map to a variable type; everything else is None.
    static VariableType typeForProp(const Prop* prop);
    static bool isExposable(const Prop* prop)
    {
        return typeForProp(prop) != VariableType::None;
    }
    static QString typeLabel(VariableType type);
    // "float", "int", ... as stored in the file; None for anything unknown.
    static QString typeToString(VariableType type);
    static VariableType typeFromJson(const QJsonValue& value);

    // A new variable (fresh id, no links) whose value and bounds start as the
    // prop's. Returns null if the prop can't be exposed.
    static TextureVariablePtr createFromProp(const Prop* prop,
                                             const QString& name,
                                             const QString& displayName);

    // Whether `prop` can link to this variable: same type, and for enums the
    // same option list.
    bool accepts(const Prop* prop) const;

    bool hasLink(const QString& nodeId, const QString& propName) const;

    QVariant value() const;
    // Sets the value, clamped to the bounds/enabled options.
    void setValue(const QVariant& value);
    // Clamp the current value into the bounds / enabled options.
    void clampValue();

    // Options the UI offers: `options` if set, otherwise all of the prop's.
    QList<EnumOption> effectiveOptions() const;

    // Write this variable's value into a linked prop.
    void writeTo(Prop* target) const;

    QJsonObject toJson() const;
    // Null when the entry is unusable (unknown type, not an object).
    static TextureVariablePtr fromJson(const QJsonObject& obj);

    TextureVariablePtr clone() const;
};

namespace variables {

// An identifier ([a-zA-Z_][a-zA-Z0-9_]*) derived from a display name.
QString sanitizeName(const QString& displayName);

// `base`, or base_2, base_3, ... — whichever isn't in `taken`.
QString uniqueName(const QString& base, const QStringList& taken);

using PropResolver =
    std::function<Prop*(const QString& nodeId, const QString& propName)>;

// Make loaded variables consistent with the project:
// - drop links whose node/prop is missing or doesn't match the variable
// - drop links to a prop already linked by an earlier variable
// - make names valid and unique
// - clamp values and drop invalid enum options
void validate(QList<TextureVariablePtr>& vars, const PropResolver& resolve);

QJsonArray toJson(const QList<TextureVariablePtr>& vars);
// Parses what it can; unusable entries are skipped.
QList<TextureVariablePtr> fromJson(const QJsonArray& array);

QList<TextureVariablePtr> cloneAll(const QList<TextureVariablePtr>& vars);

} // namespace variables
