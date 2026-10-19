#include "props.h"
#include "variables.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QtTest>

// Texture variables: JSON round trip, tolerant loading of old/hand-edited
// files, prop compatibility, clamping and writing values into linked props.
class TestVariables : public QObject {
    Q_OBJECT

private slots:
    void roundTripsEveryType();
    void roundTripsRichTypes();
    void writesTypeAsString();
    void presetsCaptureApplyAndMatch();
    void presetsRoundTripAndDropStaleValues();
    void readsElectronFormat();
    void createFromPropCopiesValueAndBounds();
    void acceptsOnlyCompatibleProps();
    void clampsToBoundsAndEnabledOptions();
    void validateDropsBadLinksAndFixesNames();
    void writeToUpdatesLinkedProp();
    void sanitizesNames();
    void cleanup()
    {
        qDeleteAll(props);
        props.clear();
    }

private:
    // A tiny stand-in for a project: "nodeId/propName" -> prop.
    QMap<QString, Prop*> props;
    variables::PropResolver resolver()
    {
        return [this](const QString& nodeId, const QString& propName) {
            return props.value(nodeId + "/" + propName, nullptr);
        };
    }
};

static FloatProp* floatProp(double value, double min, double max)
{
    auto p = new FloatProp();
    p->name = "strength";
    p->displayName = "Strength";
    p->value = value;
    p->minValue = min;
    p->maxValue = max;
    p->step = 0.01;
    return p;
}

static EnumProp* enumProp(QList<QString> values, int index = 0)
{
    auto p = new EnumProp();
    p->name = "type";
    p->displayName = "Type";
    p->values = values;
    p->index = index;
    return p;
}

void TestVariables::roundTripsEveryType()
{
    QList<TextureVariablePtr> vars;

    auto f = floatProp(0.3, 0, 2);
    vars.append(TextureVariable::createFromProp(f, "rough", "Roughness"));
    vars.last()->links.append(VariableLink{"n1", "strength"});
    delete f;

    auto i = new IntProp();
    i->value = 4; i->minValue = 1; i->maxValue = 10; i->step = 1;
    vars.append(TextureVariable::createFromProp(i, "count", "Count"));
    delete i;

    auto b = new BoolProp();
    b->value = true;
    vars.append(TextureVariable::createFromProp(b, "invert", "Invert"));
    delete b;

    auto e = enumProp({"A", "B", "C"}, 2);
    vars.append(TextureVariable::createFromProp(e, "mode", "Mode"));
    vars.last()->options = {{0, "First"}, {2, "Third"}};
    delete e;

    auto c = new ColorProp();
    c->value = QColor::fromRgbF(1, 0.5, 0.25, 1);
    vars.append(TextureVariable::createFromProp(c, "tint", "Tint"));
    delete c;

    // Through text, like a saved file.
    auto text = QJsonDocument(variables::toJson(vars)).toJson();
    auto loaded = variables::fromJson(QJsonDocument::fromJson(text).array());

    QCOMPARE(loaded.size(), 5);
    for (int n = 0; n < 5; n++) {
        QCOMPARE(loaded[n]->id, vars[n]->id);
        QCOMPARE(loaded[n]->type, vars[n]->type);
        QCOMPARE(loaded[n]->name(), vars[n]->name());
        QCOMPARE(loaded[n]->displayName(), vars[n]->displayName());
        QCOMPARE(loaded[n]->links, vars[n]->links);
        QCOMPARE(loaded[n]->options, vars[n]->options);
    }
    QCOMPARE(loaded[0]->value().toDouble(), 0.3);
    QCOMPARE(static_cast<FloatProp*>(loaded[0]->prop)->maxValue, 2.0);
    QCOMPARE(loaded[1]->value().toInt(), 4);
    QCOMPARE(loaded[2]->value().toBool(), true);
    QCOMPARE(loaded[3]->value().toInt(), 2);
    QCOMPARE(static_cast<EnumProp*>(loaded[3]->prop)->values,
             QList<QString>({"A", "B", "C"}));
    // QColor keeps 16 bits per channel.
    QVERIFY(qAbs(loaded[4]->value().value<QColor>().greenF() - 0.5f) < 1e-3f);
}

void TestVariables::roundTripsRichTypes()
{
    QList<TextureVariablePtr> vars;

    auto g = new GradientProp();
    g->value = Gradient();
    g->value.addPoint(GradientPoint(0.0f, QColor(255, 0, 0)));
    g->value.addPoint(GradientPoint(0.4f, QColor(0, 255, 0)));
    g->value.addPoint(GradientPoint(1.0f, QColor(0, 0, 255)));
    vars.append(TextureVariable::createFromProp(g, "ramp", "Ramp"));

    auto c = new CurveProp();
    c->value.addPoint(0.5f, 0.8f);
    vars.append(TextureVariable::createFromProp(c, "shape", "Shape"));

    auto img = new ImageProp();
    QImage image(4, 2, QImage::Format_RGBA8888);
    image.fill(QColor(10, 20, 30));
    img->value = image;
    vars.append(TextureVariable::createFromProp(img, "decal", "Decal"));

    auto str = new StringProp();
    str->value = "hello";
    vars.append(TextureVariable::createFromProp(str, "label", "Label"));

    auto text = QJsonDocument(variables::toJson(vars)).toJson();
    auto loaded = variables::fromJson(QJsonDocument::fromJson(text).array());
    QCOMPARE(loaded.size(), 4);

    auto lg = loaded[0]->value().value<Gradient>();
    QCOMPARE(loaded[0]->type, VariableType::Gradient);
    QCOMPARE(lg.points.size(), 3);
    QCOMPARE(lg.points[1].position, 0.4f);

    QCOMPARE(loaded[1]->type, VariableType::Curve);
    QCOMPARE(loaded[1]->value().value<Curve>().points.size(), 3);

    QCOMPARE(loaded[2]->type, VariableType::Image);
    auto li = loaded[2]->value().value<QImage>();
    QCOMPARE(li.size(), QSize(4, 2));
    QCOMPARE(QColor(li.pixel(1, 1)).green(), 20);
    // The variable's copy never shares the node prop's GL texture.
    QVERIFY(static_cast<ImageProp*>(vars[2]->prop)->texture == nullptr);

    QCOMPARE(loaded[3]->type, VariableType::String);
    QCOMPARE(loaded[3]->value().toString(), QString("hello"));

    // A linked prop of the same type takes the value.
    auto target = new GradientProp();
    loaded[0]->writeTo(target);
    QCOMPARE(target->value.points.size(), 3);

    for (Prop* p : QList<Prop*>{g, c, img, str, target})
        delete p;
}

void TestVariables::writesTypeAsString()
{
    auto f = floatProp(0.5, 0, 1);
    auto var = TextureVariable::createFromProp(f, "x", "X");
    delete f;
    QCOMPARE(var->toJson()["type"].toString(), QString("float"));

    for (int t = 1; t <= 9; t++) {
        auto type = (VariableType)t;
        auto name = TextureVariable::typeToString(type);
        QVERIFY(!name.isEmpty());
        QCOMPARE(TextureVariable::typeFromJson(QJsonValue(name)), type);
    }
    // Legacy numeric types: only Electron's 1-5.
    QCOMPARE(TextureVariable::typeFromJson(QJsonValue(5)), VariableType::Color);
    QCOMPARE(TextureVariable::typeFromJson(QJsonValue(6)), VariableType::None);
    QCOMPARE(TextureVariable::typeFromJson(QJsonValue("nope")), VariableType::None);
}

void TestVariables::presetsCaptureApplyAndMatch()
{
    auto f = floatProp(0.5, 0, 1);
    auto c = new ColorProp();
    c->value = QColor::fromRgbF(0.55, 0.2, 0.1, 1);
    QList<TextureVariablePtr> vars = {TextureVariable::createFromProp(f, "rough", "Rough"),
                                      TextureVariable::createFromProp(c, "tint", "Tint")};
    delete f;
    delete c;

    auto saved = variables::captureValues(vars);
    QVERIFY(variables::valuesMatch(vars, saved));

    // Hand-written preset values: 0.55 reads back from QColor as 0.5500076,
    // which must still count as a match once applied.
    QMap<QString, QJsonValue> preset;
    preset[vars[0]->id] = 0.25;
    QJsonObject color{{"r", 0.55}, {"g", 0.3}, {"b", 0.1}, {"a", 1.0}};
    preset[vars[1]->id] = color;
    preset["no-such-variable"] = 3; // ignored

    QVERIFY(!variables::valuesMatch(vars, preset));
    auto changed = variables::applyValues(vars, preset);
    QCOMPARE(changed.size(), 2);
    QCOMPARE(vars[0]->value().toDouble(), 0.25);
    QVERIFY(variables::valuesMatch(vars, preset));

    // Out-of-bounds preset values clamp.
    preset[vars[0]->id] = 7.0;
    variables::applyValues(vars, preset);
    QCOMPARE(vars[0]->value().toDouble(), 1.0);

    // A preset that leaves a variable out keeps its value, and doesn't match.
    QMap<QString, QJsonValue> partial;
    partial[vars[0]->id] = 0.5;
    variables::applyValues(vars, partial);
    QCOMPARE(vars[0]->value().toDouble(), 0.5);
    QVERIFY(!variables::valuesMatch(vars, partial));
}

void TestVariables::presetsRoundTripAndDropStaleValues()
{
    auto f = floatProp(0.5, 0, 1);
    QList<TextureVariablePtr> vars = {TextureVariable::createFromProp(f, "rough", "Rough")};
    delete f;

    VariablePreset preset;
    preset.id = "p1";
    preset.name = "Glossy";
    preset.values[vars[0]->id] = 0.1;
    preset.values["deleted-variable"] = 0.9;

    auto json = variables::presetsToJson({preset}, vars);
    auto loaded = variables::presetsFromJson(json);
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded[0].id, QString("p1"));
    QCOMPARE(loaded[0].name, QString("Glossy"));
    QCOMPARE(loaded[0].values.size(), 1); // stale entry dropped on save
    QCOMPARE(loaded[0].values.value(vars[0]->id).toDouble(), 0.1);

    // Duplicate ids and missing names are fixed on load.
    auto dupes = variables::presetsFromJson(QJsonDocument::fromJson(
        R"([{"id":"a","name":"One","values":{}},{"id":"a","values":{}}])").array());
    QCOMPARE(dupes.size(), 2);
    QVERIFY(dupes[0].id != dupes[1].id);
    QCOMPARE(dupes[1].name, QString("Preset"));
}

void TestVariables::readsElectronFormat()
{
    // What the Electron 0.3 app wrote: the Property object serialized as-is.
    auto json = QJsonDocument::fromJson(R"([{
        "id": "v1", "type": 1,
        "property": { "name": "scale", "displayName": "Scale", "type": "float",
                      "value": 2.5, "minValue": 0, "maxValue": 10, "step": 0.1 },
        "linkedProperties": [ { "nodeId": "n1", "name": "scale" } ]
    }])").array();

    auto vars = variables::fromJson(json);
    QCOMPARE(vars.size(), 1);
    QCOMPARE(vars[0]->type, VariableType::Float);
    QCOMPARE(vars[0]->name(), QString("scale"));
    QCOMPARE(vars[0]->value().toDouble(), 2.5);
    QCOMPARE(vars[0]->links.size(), 1);
    QCOMPARE(vars[0]->links[0].propName, QString("scale"));
}

void TestVariables::createFromPropCopiesValueAndBounds()
{
    auto f = floatProp(0.7, 0.1, 0.9);
    auto var = TextureVariable::createFromProp(f, "x", "X");
    QVERIFY(var);
    QVERIFY(!var->id.isEmpty());
    QVERIFY(var->links.isEmpty());
    auto vp = static_cast<FloatProp*>(var->prop);
    QCOMPARE(vp->value, 0.7);
    QCOMPARE(vp->minValue, 0.1);
    QCOMPARE(vp->maxValue, 0.9);
    // The variable owns a copy.
    QVERIFY(var->prop != f);
    delete f;

    // Every prop type the node libraries use can be exposed.
    auto s = new StringProp();
    QVERIFY(TextureVariable::isExposable(s));
    delete s;
}

void TestVariables::acceptsOnlyCompatibleProps()
{
    auto f = floatProp(0.5, 0, 1);
    auto var = TextureVariable::createFromProp(f, "x", "X");
    auto other = floatProp(0.1, 0, 5);
    auto i = new IntProp();
    QVERIFY(var->accepts(other));
    QVERIFY(!var->accepts(i));

    auto e1 = enumProp({"A", "B"});
    auto e2 = enumProp({"A", "B"});
    auto e3 = enumProp({"A", "C"});
    auto evar = TextureVariable::createFromProp(e1, "m", "M");
    QVERIFY(evar->accepts(e2));
    QVERIFY(!evar->accepts(e3));
    QVERIFY(!evar->accepts(f));

    for (Prop* p : QList<Prop*>{f, other, i, e1, e2, e3})
        delete p;
}

void TestVariables::clampsToBoundsAndEnabledOptions()
{
    auto f = floatProp(0.5, 0, 1);
    auto var = TextureVariable::createFromProp(f, "x", "X");
    var->setValue(3.0);
    QCOMPARE(var->value().toDouble(), 1.0);
    var->setValue(-1.0);
    QCOMPARE(var->value().toDouble(), 0.0);
    delete f;

    auto e = enumProp({"A", "B", "C", "D"}, 1);
    auto evar = TextureVariable::createFromProp(e, "m", "M");
    evar->options = {{2, "C"}, {3, "D"}};
    evar->clampValue();
    QCOMPARE(evar->value().toInt(), 2); // 1 isn't enabled -> first enabled
    evar->setValue(9);
    QCOMPARE(evar->value().toInt(), 3); // out of range -> last option, which is enabled
    delete e;
}

void TestVariables::validateDropsBadLinksAndFixesNames()
{
    props["n1/strength"] = floatProp(0.2, 0, 1);
    props["n2/strength"] = floatProp(0.4, 0, 1);
    props["n3/type"] = enumProp({"A", "B"});

    auto json = QJsonDocument::fromJson(R"([
        { "id": "a", "type": 1, "property": { "name": "rough", "value": 0.5 },
          "linkedProperties": [ { "nodeId": "n1", "name": "strength" },
                                { "nodeId": "missing", "name": "strength" },
                                { "nodeId": "n3", "name": "type" } ] },
        { "id": "b", "type": 1, "property": { "name": "rough", "value": 0.1 },
          "linkedProperties": [ { "nodeId": "n1", "name": "strength" },
                                { "nodeId": "n2", "name": "strength" } ] },
        { "id": "c", "type": 99, "property": { "name": "nope" } },
        { "id": "d", "type": 4, "property": { "name": "9 Bad Name!", "index": 0, "values": ["A","B"] },
          "options": [ { "index": 5, "label": "x" }, { "index": 1, "label": "" } ] }
    ])").array();

    auto vars = variables::fromJson(json);
    QCOMPARE(vars.size(), 3); // unknown type dropped
    variables::validate(vars, resolver());

    // Missing node and type-mismatched links dropped.
    QCOMPARE(vars[0]->links, QList<VariableLink>({{"n1", "strength"}}));
    // n1/strength already linked by the first variable.
    QCOMPARE(vars[1]->links, QList<VariableLink>({{"n2", "strength"}}));
    // Duplicate and invalid names fixed.
    QCOMPARE(vars[0]->name(), QString("rough"));
    QCOMPARE(vars[1]->name(), QString("rough_2"));
    QCOMPARE(vars[2]->name(), QString("_9_bad_name"));
    // Out-of-range option dropped, empty label filled from the prop.
    QCOMPARE(vars[2]->options, QList<EnumOption>({{1, "B"}}));
    // Value moved onto an enabled option.
    QCOMPARE(vars[2]->value().toInt(), 1);
}

void TestVariables::writeToUpdatesLinkedProp()
{
    auto source = floatProp(0.25, 0, 1);
    auto var = TextureVariable::createFromProp(source, "x", "X");
    auto target = floatProp(0.9, 0, 1);
    var->setValue(0.6);
    var->writeTo(target);
    QCOMPARE(target->value, 0.6);

    auto e = enumProp({"A", "B"});
    var->writeTo(e); // incompatible: untouched
    QCOMPARE(e->index, 0);

    for (Prop* p : QList<Prop*>{source, target, e})
        delete p;
}

void TestVariables::sanitizesNames()
{
    QCOMPARE(variables::sanitizeName("Roughness"), QString("roughness"));
    QCOMPARE(variables::sanitizeName("  Brick Height (px) "), QString("brick_height_px"));
    QCOMPARE(variables::sanitizeName("3D Scale"), QString("_3d_scale"));
    QCOMPARE(variables::sanitizeName("!!!"), QString("variable"));
    QCOMPARE(variables::uniqueName("x", {"x", "x_2"}), QString("x_3"));
    QCOMPARE(variables::uniqueName("y", {"x"}), QString("y"));
}

QTEST_MAIN(TestVariables)
#include "tst_variables.moc"
