#include "variableswidget.h"

#include "../../models.h"
#include "../../props.h"
#include "../../undo/variableschangecommand.h"
#include "../properties/curvepropwidget.h"
#include "../properties/propwidgets.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
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
    layout->setContentsMargins(6, 6, 6, 6);

    list = new QListWidget(this);
    list->setObjectName("VariablesList");
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(list);

    emptyLabel = new QLabel(
        "No variables yet.\n\nUse the link button on a property to expose it "
        "as a texture variable.",
        this);
    emptyLabel->setObjectName("VariablesEmpty");
    emptyLabel->setWordWrap(true);
    emptyLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(emptyLabel);

    auto buttons = new QHBoxLayout();
    upButton = new QPushButton("Move Up", this);
    downButton = new QPushButton("Move Down", this);
    deleteButton = new QPushButton("Delete", this);
    for (auto b : {upButton, downButton, deleteButton}) {
        b->setProperty("size", "small");
        buttons->addWidget(b);
    }
    buttons->addStretch(1);
    layout->addLayout(buttons);

    scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    editorHost = new QWidget();
    editorLayout = new QVBoxLayout(editorHost);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    scroll->setWidget(editorHost);
    layout->addWidget(scroll, 1);

    connect(list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (rebuilding || !project)
            return;
        selectedId = (row >= 0 && row < project->variables.size())
                         ? project->variables[row]->id
                         : QString();
        rebuildEditor();
    });
    connect(upButton, &QPushButton::clicked, this, [this]() { moveSelected(-1); });
    connect(downButton, &QPushButton::clicked, this, [this]() { moveSelected(1); });
    connect(deleteButton, &QPushButton::clicked, this, [this]() { deleteSelected(); });

    refresh();
}

void VariablesWidget::setProject(const TextureProjectPtr& project)
{
    this->project = project;
    selectedId.clear();
    refresh();
}

TextureVariablePtr VariablesWidget::selected() const
{
    return project ? project->getVariableById(selectedId) : TextureVariablePtr();
}

void VariablesWidget::refresh()
{
    if (project && !project->getVariableById(selectedId))
        selectedId = project->variables.isEmpty() ? QString()
                                                  : project->variables.first()->id;
    rebuildList();
    rebuildEditor();
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
    selectedId = id;
    refresh();
}

void VariablesWidget::rebuildList()
{
    builtSignature = signature();
    rebuilding = true;
    list->clear();
    int row = -1;
    if (project) {
        for (int i = 0; i < project->variables.size(); i++) {
            auto var = project->variables[i];
            QString links = var->links.isEmpty()
                                ? "unlinked"
                                : QString("%1 link%2")
                                      .arg(var->links.size())
                                      .arg(var->links.size() == 1 ? "" : "s");
            list->addItem(QString("%1   ·   %2, %3")
                              .arg(var->displayName(),
                                   TextureVariable::typeLabel(var->type), links));
            if (var->id == selectedId)
                row = i;
        }
    }
    list->setCurrentRow(row);
    rebuilding = false;

    bool any = project && !project->variables.isEmpty();
    // Size the list to its rows (up to 6, then it scrolls) so the editor
    // below gets the room.
    int rowHeight = list->sizeHintForRow(0) > 0 ? list->sizeHintForRow(0) : 22;
    int rows = qBound(1, list->count(), 6);
    list->setFixedHeight(rows * rowHeight + 2 * list->frameWidth() + 4);
    list->setVisible(any);
    emptyLabel->setVisible(!any);
    upButton->setEnabled(row > 0);
    downButton->setEnabled(any && row >= 0 && row < project->variables.size() - 1);
    deleteButton->setEnabled(row >= 0);
    upButton->setVisible(any);
    downButton->setVisible(any);
    deleteButton->setVisible(any);
}

void VariablesWidget::rebuildEditor()
{
    // Keep the scroll position across rebuilds of the same variable (e.g.
    // after toggling an option near the bottom).
    int scrollPos = selectedId == lastBuiltId ? scroll->verticalScrollBar()->value() : 0;
    lastBuiltId = selectedId;
    QTimer::singleShot(0, this, [this, scrollPos]() {
        scroll->verticalScrollBar()->setValue(scrollPos);
    });

    // Clear the old editor.
    while (editorLayout->count() > 0) {
        auto item = editorLayout->takeAt(0);
        if (auto w = item->widget()) {
            w->hide();
            w->deleteLater();
        }
        delete item;
    }
    // Freed with its editor widget (see createValueEditor).
    editorProp = nullptr;

    auto var = selected();
    if (!var) {
        editorLayout->addStretch(1);
        return;
    }

    // --- names ---
    auto names = new QWidget();
    auto form = new QFormLayout(names);
    form->setContentsMargins(0, 8, 0, 0);

    auto displayEdit = new QLineEdit(var->displayName());
    form->addRow("Display Name", displayEdit);

    auto nameEdit = new QLineEdit(var->name());
    nameEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[A-Za-z_][A-Za-z0-9_]*"), nameEdit));
    nameEdit->setToolTip("Identifier used by the web runtime and API. Letters, "
                         "digits and underscores.");
    form->addRow("Name", nameEdit);

    form->addRow("Type", new QLabel(TextureVariable::typeLabel(var->type)));
    editorLayout->addWidget(names);

    connect(displayEdit, &QLineEdit::editingFinished, this, [=]() {
        auto text = displayEdit->text().trimmed();
        auto current = selected();
        if (!current || text.isEmpty() || text == current->displayName())
            return;
        editSelected("Rename Variable", [&](TextureVariablePtr v) {
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
        auto current = selected();
        if (!current || text.isEmpty() || text == current->name())
            return;
        if (project->variableNames().contains(text)) {
            nameEdit->setText(current->name());
            nameEdit->setToolTip(QString("\"%1\" is already used by another variable.").arg(text));
            return;
        }
        editSelected("Rename Variable", [&](TextureVariablePtr v) { v->prop->name = text; });
    });

    // --- value ---
    auto valueHeader = new QLabel("Value");
    valueHeader->setObjectName("PropSectionTitle");
    editorLayout->addWidget(valueHeader);
    if (auto editor = createValueEditor(var)) {
        // The widget points at editorProp; free them together.
        Prop* owned = editorProp;
        connect(editor, &QObject::destroyed, [owned]() { delete owned; });
        editorLayout->addWidget(editor);
    }

    // --- bounds / options ---
    if (var->type == VariableType::Float || var->type == VariableType::Int) {
        auto header = new QLabel("Bounds");
        header->setObjectName("PropSectionTitle");
        editorLayout->addWidget(header);
        editorLayout->addWidget(createBoundsEditor(var));
    }
    if (var->type == VariableType::Enum) {
        auto header = new QLabel("Options");
        header->setObjectName("PropSectionTitle");
        editorLayout->addWidget(header);
        editorLayout->addWidget(createOptionsEditor(var));
    }

    // --- links ---
    auto linksHeader = new QLabel("Linked Properties");
    linksHeader->setObjectName("PropSectionTitle");
    editorLayout->addWidget(linksHeader);
    editorLayout->addWidget(createLinksEditor(var));

    editorLayout->addStretch(1);
}

void VariablesWidget::editSelected(
    const QString& text, const std::function<void(TextureVariablePtr)>& change,
    bool mergeValue, bool rebuild)
{
    auto var = selected();
    if (!var || !project)
        return;

    VariablesEdit edit(project, renderer, text, mergeValue ? var->id : QString());
    edit.touchVariable(var);
    change(var);
    var->clampValue();
    project->applyVariable(var);
    if (undoStack)
        undoStack->push(edit.finish());

    if (rebuild) {
        // Deferred: this may run inside a signal of a widget being rebuilt.
        QTimer::singleShot(0, this, [this]() { refresh(); });
    }
    else {
        rebuildList();
    }
    emit valueChanged();
    emit variablesChanged();
}

QWidget* VariablesWidget::createValueEditor(const TextureVariablePtr& var)
{
    switch (var->type) {
    case VariableType::Float: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new FloatPropWidget();
        w->setProp(static_cast<FloatProp*>(editorProp));
        connect(w, &FloatPropWidget::valueChanged, this, [this](float value) {
            editSelected("Change Variable", [&](TextureVariablePtr v) { v->setValue((double)value); },
                         true, false);
        });
        return w;
    }
    case VariableType::Int: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new IntPropWidget();
        w->setProp(static_cast<IntProp*>(editorProp));
        connect(w, &IntPropWidget::valueChanged, this, [this](long value) {
            editSelected("Change Variable", [&](TextureVariablePtr v) { v->setValue((int)value); },
                         true, false);
        });
        return w;
    }
    case VariableType::Bool: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new BoolPropWidget();
        w->setProp(static_cast<BoolProp*>(editorProp));
        connect(w, &BoolPropWidget::valueChanged, this, [this](bool value) {
            editSelected("Change Variable", [&](TextureVariablePtr v) { v->setValue(value); },
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
        connect(w, &EnumPropWidget::valueChanged, this, [this, options](int row) {
            if (row < 0 || row >= options.size())
                return;
            int index = options[row].index;
            editSelected("Change Variable", [&](TextureVariablePtr v) { v->setValue(index); },
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
        connect(w, &ColorPropWidget::valuePreviewed, this, [this](const QColor& color) {
            auto v = selected();
            if (!v)
                return;
            v->setValue(color);
            project->applyVariable(v);
            emit valueChanged();
        });
        connect(w, &ColorPropWidget::valueChanged, this, [this](const QColor& color) {
            editSelected("Change Variable", [&](TextureVariablePtr v) { v->setValue(color); },
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
        connect(w, &GradientPropWidget::valuePreviewed, this, [this](const Gradient& g) {
            auto v = selected();
            if (!v)
                return;
            v->setValue(QVariant::fromValue(g));
            project->applyVariable(v);
            emit valueChanged();
        });
        connect(w, &GradientPropWidget::valueChanged, this, [this](const Gradient& g) {
            editSelected("Change Variable",
                         [&](TextureVariablePtr v) { v->setValue(QVariant::fromValue(g)); },
                         false, false);
        });
        return w;
    }
    case VariableType::Curve: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new CurvePropWidget(static_cast<CurveProp*>(editorProp));
        connect(w, &CurvePropWidget::valueChanged, this, [this](const Curve& c) {
            editSelected("Change Variable",
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
        connect(w, &ImagePropWidget::valueChanged, this, [this](const QImage& image) {
            editSelected("Change Variable", [&](TextureVariablePtr v) { v->setValue(image); },
                         false, false);
        });
        return w;
    }
    case VariableType::String: {
        editorProp = var->prop->clone();
        editorProp->displayName = var->displayName();
        auto w = new StringPropWidget();
        w->setProp(static_cast<StringProp*>(editorProp));
        connect(w, &StringPropWidget::valueChanged, this, [this](const QString& text) {
            editSelected("Change Variable", [&](TextureVariablePtr v) { v->setValue(text); },
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
            editSelected("Change Variable Bounds", [&](TextureVariablePtr v) {
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
            editSelected("Change Variable Bounds", [&](TextureVariablePtr v) {
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

    auto commit = [this, rows, values]() {
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
            QTimer::singleShot(0, this, [this]() { refresh(); });
            return;
        }
        editSelected("Change Variable Options", [&](TextureVariablePtr v) {
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
        select->setToolTip("Select this node");
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
        connect(unlink, &QPushButton::clicked, this, [this, l, text]() {
            editSelected(QString("Unlink %1").arg(text),
                         [&](TextureVariablePtr v) { v->links.removeAll(l); });
        });
    }
    return host;
}

void VariablesWidget::moveSelected(int delta)
{
    auto var = selected();
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
    refresh();
    emit variablesChanged();
}

void VariablesWidget::deleteSelected()
{
    auto var = selected();
    if (!var)
        return;

    // Linked props keep their current value.
    VariablesEdit edit(project, renderer,
                       QString("Delete Variable %1").arg(var->displayName()));
    project->variables.removeAll(var);
    if (undoStack)
        undoStack->push(edit.finish());
    selectedId.clear();
    refresh();
    emit variablesChanged();
}
