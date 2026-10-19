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

    auto s = new StringProp();
    QVERIFY(!TextureVariable::createFromProp(s, "s", "S"));
    QVERIFY(!TextureVariable::isExposable(s));
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
