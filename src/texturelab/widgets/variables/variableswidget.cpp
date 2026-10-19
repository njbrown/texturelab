#include "variableswidget.h"

#include "../../models.h"
#include "../../props.h"
#include "../../undo/variableschangecommand.h"
#include "../properties/accordionwidget.h"
#include "../properties/curvepropwidget.h"
#include "../properties/propwidgets.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

VariablesWidget::VariablesWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("VariablesPanel");

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    emptyLabel = new QLabel(
        "No variables yet.\n\nUse the link button on a property to expose it "
        "as a texture variable.",
        this);
    emptyLabel->setObjectName("VariablesEmpty");
    emptyLabel->setWordWrap(true);
    emptyLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(emptyLabel);

    scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    // Sections always fit the dock's width.
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sectionsHost = new QWidget();
    sectionsLayout = new QVBoxLayout(sectionsHost);
    sectionsLayout->setContentsMargins(0, 0, 0, 0);
    sectionsLayout->setSpacing(0);
    scroll->setWidget(sectionsHost);
    layout->addWidget(scroll, 1);

    refresh();
}

void VariablesWidget::setProject(const TextureProjectPtr& project)
{
    this->project = project;
    expanded.clear();
    scrollToId.clear();
    // A short list opens fully; a long one starts collapsed.
    if (project && project->variables.size() <= 3)
        for (const auto& var : project->variables)
            expanded.insert(var->id);
    scroll->verticalScrollBar()->setValue(0);
    refresh();
}

QString VariablesWidget::signature() const
{
    if (!project)
        return {};
    QString sig;
    for (const auto& var : project->variables) {
        sig += var->id + '|' + var->name() + '|' + var->displayName() + '|';
        for (const auto& link : var->links)
            sig += link.nodeId + '/' + link.propName + ',';
        sig += ';';
    }
    return sig;
}

void VariablesWidget::refreshIfChanged()
{
    if (signature() != builtSignature)
        refresh();
}

void VariablesWidget::selectVariable(const QString& id)
{
    expanded.insert(id);
    scrollToId = id;
    refresh();
}

QString VariablesWidget::sectionTitle(const TextureVariablePtr& var) const
{
    QString links = var->links.isEmpty()
                        ? "unlinked"
                        : QString("%1 link%2").arg(var->links.size()).arg(var->links.size() == 1 ? "" : "s");
    return QString("%1   ·   %2, %3")
        .arg(var->displayName(), TextureVariable::typeLabel(var->type), links);
}

void VariablesWidget::refresh()
{
    builtSignature = signature();
    int scrollPos = scroll->verticalScrollBar()->value();

    while (sectionsLayout->count() > 0) {
        auto item = sectionsLayout->takeAt(0);
        if (auto w = item->widget()) {
            w->hide();
            w->deleteLater();
        }
        delete item;
    }

    bool any = project && !project->variables.isEmpty();
    emptyLabel->setVisible(!any);
    scroll->setVisible(any);
    if (!any)
        return;

    // Forget sections whose variable is gone.
    QSet<QString> ids;
    for (const auto& var : project->variables)
        ids.insert(var->id);
    expanded.intersect(ids);

    QWidget* target = nullptr;
    for (int i = 0; i < project->variables.size(); i++) {
        auto var = project->variables[i];
        auto section = createSection(var, i);
        sectionsLayout->addWidget(section);
        if (var->id == scrollToId)
            target = section;
    }
    sectionsLayout->addStretch(1);

    // After layout: restore the scroll position, or bring the requested
    // section into view.
    QTimer::singleShot(0, this, [this, scrollPos, target]() {
        if (target)
            scroll->ensureWidgetVisible(target, 0, 0);
        else
            scroll->verticalScrollBar()->setValue(scrollPos);
    });
    scrollToId.clear();
}

QWidget* VariablesWidget::createSection(const TextureVariablePtr& var, int index)
{
    const QString id = var->id;
    auto accordion = new AccordionWidget(sectionTitle(var), !expanded.contains(id));
    connect(accordion, &AccordionWidget::toggled, this, [this, id](bool collapsed) {
        if (collapsed)
            expanded.remove(id);
        else
            expanded.insert(id);
    });

    auto body = new QWidget();
    auto layout = new QVBoxLayout(body);
    layout->setContentsMargins(8, 4, 12, 10);

    // --- names ---
    auto names = new QWidget();
    auto form = new QFormLayout(names);
    form->setContentsMargins(0, 4, 0, 0);

    auto displayEdit = new QLineEdit(var->displayName());
    form->addRow("Display Name", displayEdit);

    auto nameEdit = new QLineEdit(var->name());
    nameEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[A-Za-z_][A-Za-z0-9_]*"), nameEdit));
    nameEdit->setToolTip("Identifier used by the web runtime and API. Letters, "
                         "digits and underscores.");
    form->addRow("Name", nameEdit);
    layout->addWidget(names);

    connect(displayEdit, &QLineEdit::editingFinished, this, [=]() {
        auto text = displayEdit->text().trimmed();
        auto current = project->getVariableById(id);
        if (!current || text.isEmpty() || text == current->displayName())
            return;
        editVariable(id, "Rename Variable", [&](TextureVariablePtr v) {
            // Keep deriving the name from the display name until the user
            // sets one by hand.
            auto others = project->variableNames();
            others.removeAll(v->name());
            auto derivedOld =
                variables::uniqueName(variables::sanitizeName(v->displayName()), others);
            if (v->name() == derivedOld)
                v->prop->name = variables::uniqueName(variables::sanitizeName(text), others);
            v->prop->displayName = text;
        });
    });

    connect(nameEdit, &QLineEdit::editingFinished, this, [=]() {
        auto text = nameEdit->text();
        auto current = project->getVariableById(id);
        if (!current || text.isEmpty() || text == current->name())
            return;
        if (project->variableNames().contains(text)) {
            nameEdit->setText(current->name());
            nameEdit->setToolTip(QString("\"%1\" is already used by another variable.").arg(text));
            return;
        }
        editVariable(id, "Rename Variable", [&](TextureVariablePtr v) { v->prop->name = text; });
    });

    // --- value ---
    if (auto editor = createValueEditor(var)) {
        // The widget points at editorProp; free them together.
        Prop* owned = editorProp;
        editorProp = nullptr;
        connect(editor, &QObject::destroyed, [owned]() { delete owned; });
        layout->addWidget(editor);
    }

    // --- bounds / options ---
    auto addHeader = [&](const QString& text) {
        auto header = new QLabel(text);
        header->setObjectName("PropSectionTitle");
        layout->addWidget(header);
    };
    if (var->type == VariableType::Float || var->type == VariableType::Int) {
        addHeader("Bounds");
        layout->addWidget(createBoundsEditor(var));
    }
    if (var->type == VariableType::Enum) {
        addHeader("Options");
        layout->addWidget(createOptionsEditor(var));
    }

    // --- links ---
    addHeader("Linked Properties");
    layout->addWidget(createLinksEditor(var));

    // --- move / delete ---
    auto actions = new QHBoxLayout();
    actions->setContentsMargins(0, 6, 0, 0);
    auto up = new QPushButton("Up");
    auto down = new QPushButton("Down");
    auto remove = new QPushButton("Delete");
    up->setToolTip("Move this variable up");
    down->setToolTip("Move this variable down");
    for (auto b : {up, down, remove}) {
        b->setProperty("size", "small");
        b->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        actions->addWidget(b, 1);
    }
    up->setEnabled(index > 0);
    down->setEnabled(index < project->variables.size() - 1);
    remove->setToolTip("Delete this variable. Linked properties keep their current value.");
    connect(up, &QPushButton::clicked, this, [this, id]() { moveVariable(id, -1); });
    connect(down, &QPushButton::clicked, this, [this, id]() { moveVariable(id, 1); });
    connect(remove, &QPushButton::clicked, this, [this, id]() { deleteVariable(id); });
    layout->addLayout(actions);

    accordion->addWidget(body);
    return accordion;
}

void VariablesWidget::editVariable(
    const QString& id, const QString& text,
    const std::function<void(TextureVariablePtr)>& change, bool mergeValue,
    bool rebuild)
{
    auto var = project ? project->getVariableById(id) : TextureVariablePtr();
    if (!var)
        return;

    VariablesEdit edit(project, renderer, text, mergeValue ? var->id : QString());
    edit.touchVariable(var);
    change(var);
    var->clampValue();
    project->applyVariable(var);
    if (undoStack)
        undoStack->push(edit.finish());

    // Deferred: this may run inside a signal of a widget being rebuilt.
    if (rebuild)
        QTimer::singleShot(0, this, [this]() { refresh(); });
    else
        builtSignature = signature();
    emit valueChanged();
    emit variablesChanged();
}

QWidget* VariablesWidget::createValueEditor(const TextureVariablePtr& var)
{
    const QString id = var->id;
    switch (var->type) {
    case VariableType::Float: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new FloatPropWidget();
        w->setProp(static_cast<FloatProp*>(editorProp));
        connect(w, &FloatPropWidget::valueChanged, this, [this, id](float value) {
            editVariable(id, "Change Variable", [&](TextureVariablePtr v) { v->setValue((double)value); },
                         true, false);
        });
        return w;
    }
    case VariableType::Int: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new IntPropWidget();
        w->setProp(static_cast<IntProp*>(editorProp));
        connect(w, &IntPropWidget::valueChanged, this, [this, id](long value) {
            editVariable(id, "Change Variable", [&](TextureVariablePtr v) { v->setValue((int)value); },
                         true, false);
        });
        return w;
    }
    case VariableType::Bool: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new BoolPropWidget();
        w->setProp(static_cast<BoolProp*>(editorProp));
        connect(w, &BoolPropWidget::valueChanged, this, [this, id](bool value) {
            editVariable(id, "Change Variable", [&](TextureVariablePtr v) { v->setValue(value); },
                         false, false);
        });
        return w;
    }
    case VariableType::Enum: {
        // Only the enabled options, with the variable's labels. Combo row i
        // maps to option i's index in the prop's own list.
        auto options = var->effectiveOptions();
        auto enumProp = new EnumProp();
        enumProp->displayName = var->displayName();
        int current = 0;
        for (int i = 0; i < options.size(); i++) {
            enumProp->values.append(options[i].label);
            if (options[i].index == var->value().toInt())
                current = i;
        }
        enumProp->index = current;
        editorProp = enumProp;
        auto w = new EnumPropWidget();
        w->setProp(enumProp);
        connect(w, &EnumPropWidget::valueChanged, this, [this, id, options](int row) {
            if (row < 0 || row >= options.size())
                return;
            int index = options[row].index;
            editVariable(id, "Change Variable", [&](TextureVariablePtr v) { v->setValue(index); },
                         false, false);
        });
        return w;
    }
    case VariableType::Color: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new ColorPropWidget();
        w->setProp(static_cast<ColorProp*>(editorProp));
        // Live while the picker is open, without an undo step per move.
        connect(w, &ColorPropWidget::valuePreviewed, this, [this, id](const QColor& color) {
            auto v = project->getVariableById(id);
            if (!v)
                return;
            v->setValue(color);
            project->applyVariable(v);
            emit valueChanged();
        });
        connect(w, &ColorPropWidget::valueChanged, this, [this, id](const QColor& color) {
            editVariable(id, "Change Variable", [&](TextureVariablePtr v) { v->setValue(color); },
                         false, false);
        });
        return w;
    }
    case VariableType::Gradient: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new GradientPropWidget();
        w->setProp(static_cast<GradientProp*>(editorProp));
        // Live while the picker is open; one undo step when it closes.
        connect(w, &GradientPropWidget::valuePreviewed, this, [this, id](const Gradient& g) {
            auto v = project->getVariableById(id);
            if (!v)
                return;
            v->setValue(QVariant::fromValue(g));
            project->applyVariable(v);
            emit valueChanged();
        });
        connect(w, &GradientPropWidget::valueChanged, this, [this, id](const Gradient& g) {
            editVariable(id, "Change Variable",
                         [&](TextureVariablePtr v) { v->setValue(QVariant::fromValue(g)); },
                         false, false);
        });
        return w;
    }
    case VariableType::Curve: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new CurvePropWidget(static_cast<CurveProp*>(editorProp));
        connect(w, &CurvePropWidget::valueChanged, this, [this, id](const Curve& c) {
            editVariable(id, "Change Variable",
                         [&](TextureVariablePtr v) { v->setValue(QVariant::fromValue(c)); },
                         true, false);
        });
        return w;
    }
    case VariableType::Image: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        static_cast<ImageProp*>(editorProp)->texture = nullptr;
        auto w = new ImagePropWidget();
        w->setProp(static_cast<ImageProp*>(editorProp));
        connect(w, &ImagePropWidget::valueChanged, this, [this, id](const QImage& image) {
            editVariable(id, "Change Variable", [&](TextureVariablePtr v) { v->setValue(image); },
                         false, false);
        });
        return w;
    }
    case VariableType::String: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new StringPropWidget();
        w->setProp(static_cast<StringProp*>(editorProp));
        connect(w, &StringPropWidget::valueChanged, this, [this, id](const QString& text) {
            editVariable(id, "Change Variable", [&](TextureVariablePtr v) { v->setValue(text); },
                         true, false);
        });
        return w;
    }
    default:
        return nullptr;
    }
}

QWidget* VariablesWidget::createBoundsEditor(const TextureVariablePtr& var)
{
    const QString id = var->id;
    auto host = new QWidget();
    auto form = new QFormLayout(host);
    form->setContentsMargins(0, 0, 0, 0);

    if (var->type == VariableType::Float) {
        auto p = static_cast<FloatProp*>(var->prop);
        auto make = [&](double value) {
            auto s = new QDoubleSpinBox();
            s->setRange(-1e9, 1e9);
            s->setDecimals(4);
            s->setValue(value);
            return s;
        };
        auto minBox = make(p->minValue);
        auto maxBox = make(p->maxValue);
        auto stepBox = make(p->step);
        stepBox->setMinimum(0.0001);
        form->addRow("Min", minBox);
        form->addRow("Max", maxBox);
        form->addRow("Step", stepBox);

        auto commit = [=]() {
            double mn = minBox->value(), mx = maxBox->value(), st = stepBox->value();
            if (mn >= mx)
                return; // keep editing until min < max
            editVariable(id, "Change Variable Bounds", [&](TextureVariablePtr v) {
                auto fp = static_cast<FloatProp*>(v->prop);
                fp->minValue = mn;
                fp->maxValue = mx;
                fp->step = st;
            });
        };
        for (auto box : {minBox, maxBox, stepBox})
            connect(box, &QDoubleSpinBox::editingFinished, this, commit);
    }
    else {
        auto p = static_cast<IntProp*>(var->prop);
        auto make = [&](long value) {
            auto s = new QSpinBox();
            s->setRange(-1000000000, 1000000000);
            s->setValue((int)value);
            return s;
        };
        auto minBox = make(p->minValue);
        auto maxBox = make(p->maxValue);
        auto stepBox = make(p->step);
        stepBox->setMinimum(1);
        form->addRow("Min", minBox);
        form->addRow("Max", maxBox);
        form->addRow("Step", stepBox);

        auto commit = [=]() {
            int mn = minBox->value(), mx = maxBox->value(), st = stepBox->value();
            if (mn >= mx)
                return;
            editVariable(id, "Change Variable Bounds", [&](TextureVariablePtr v) {
                auto ip = static_cast<IntProp*>(v->prop);
                ip->minValue = mn;
                ip->maxValue = mx;
                ip->step = st;
            });
        };
        for (auto box : {minBox, maxBox, stepBox})
            connect(box, &QSpinBox::editingFinished, this, commit);
    }
    return host;
}

QWidget* VariablesWidget::createOptionsEditor(const TextureVariablePtr& var)
{
    const QString id = var->id;
    auto host = new QWidget();
    auto grid = new QVBoxLayout(host);
    grid->setContentsMargins(0, 0, 0, 0);

    auto values = static_cast<EnumProp*>(var->prop)->values;
    auto effective = var->effectiveOptions();

    struct Row { QCheckBox* check; QLineEdit* label; };
    auto rows = std::make_shared<QList<Row>>();

    for (int i = 0; i < values.size(); i++) {
        auto rowWidget = new QWidget();
        auto h = new QHBoxLayout(rowWidget);
        h->setContentsMargins(0, 0, 0, 0);
        auto check = new QCheckBox();
        QString label = values[i];
        bool enabled = false;
        for (const auto& o : effective)
            if (o.index == i) {
                enabled = true;
                label = o.label;
            }
        check->setChecked(enabled);
        check->setToolTip(QString("Offer \"%1\"").arg(values[i]));
        auto edit = new QLineEdit(label);
        edit->setPlaceholderText(values[i]);
        h->addWidget(check);
        h->addWidget(edit, 1);
        grid->addWidget(rowWidget);
        rows->append({check, edit});
    }

    auto commit = [this, id, rows, values]() {
        QList<EnumOption> options;
        bool allDefault = true;
        for (int i = 0; i < rows->size(); i++) {
            auto& r = (*rows)[i];
            auto label = r.label->text().trimmed();
            if (label.isEmpty())
                label = values[i];
            if (!r.check->isChecked() || label != values[i])
                allDefault = false;
            if (r.check->isChecked())
                options.append({i, label});
        }
        if (options.isEmpty()) {
            // At least one option has to stay available; undo the uncheck.
            QTimer::singleShot(0, this, [this, id]() { refresh(); });
            return;
        }
        editVariable(id, "Change Variable Options", [&](TextureVariablePtr v) {
            // Every option enabled with its own label is stored as "all".
            v->options = allDefault ? QList<EnumOption>() : options;
        });
    };
    for (const auto& r : *rows) {
        connect(r.check, &QCheckBox::toggled, this, commit);
        connect(r.label, &QLineEdit::editingFinished, this, commit);
    }
    return host;
}

QWidget* VariablesWidget::createLinksEditor(const TextureVariablePtr& var)
{
    const QString id = var->id;
    auto host = new QWidget();
    auto v = new QVBoxLayout(host);
    v->setContentsMargins(0, 0, 0, 0);

    if (var->links.isEmpty()) {
        auto none = new QLabel("Not linked to any property. Its value is kept, "
                               "but it doesn't affect the texture.");
        none->setWordWrap(true);
        none->setObjectName("VariablesEmpty");
        v->addWidget(none);
        return host;
    }

    for (const auto& link : var->links) {
        auto node = project->getNodeById(link.nodeId);
        auto prop = node ? node->getProp(link.propName) : nullptr;
        QString text = QString("%1  ›  %2")
                           .arg(node ? node->title : link.nodeId,
                                prop ? prop->displayName : link.propName);

        auto row = new QWidget();
        auto h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        auto select = new QPushButton(text);
        select->setFlat(true);
        select->setStyleSheet("text-align: left");
        select->setToolTip(QString("%1 (click to select the node)").arg(text));
        // Long node/prop names mustn't widen the panel; the tooltip has the
        // full text.
        select->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        auto unlink = new QPushButton("Unlink");
        unlink->setProperty("size", "small");
        h->addWidget(select, 1);
        h->addWidget(unlink);
        v->addWidget(row);

        QString nodeId = link.nodeId;
        VariableLink l = link;
        connect(select, &QPushButton::clicked, this, [this, nodeId]() {
            emit nodeActivated(nodeId);
        });
        connect(unlink, &QPushButton::clicked, this, [this, id, l, text]() {
            editVariable(id, QString("Unlink %1").arg(text),
                         [&](TextureVariablePtr v) { v->links.removeAll(l); });
        });
    }
    return host;
}

void VariablesWidget::moveVariable(const QString& id, int delta)
{
    auto var = project ? project->getVariableById(id) : TextureVariablePtr();
    if (!var)
        return;
    int from = project->variables.indexOf(var);
    int to = from + delta;
    if (from < 0 || to < 0 || to >= project->variables.size())
        return;

    VariablesEdit edit(project, renderer, "Reorder Variables");
    project->variables.move(from, to);
    if (undoStack)
        undoStack->push(edit.finish());
    scrollToId = id;
    QTimer::singleShot(0, this, [this]() { refresh(); });
    emit variablesChanged();
}

void VariablesWidget::deleteVariable(const QString& id)
{
    auto var = project ? project->getVariableById(id) : TextureVariablePtr();
    if (!var)
        return;

    // Linked props keep their current value.
    VariablesEdit edit(project, renderer,
                       QString("Delete Variable %1").arg(var->displayName()));
    project->variables.removeAll(var);
    if (undoStack)
        undoStack->push(edit.finish());
    QTimer::singleShot(0, this, [this]() { refresh(); });
    emit variablesChanged();
}
