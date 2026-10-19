#include "variableswidget.h"

#include "../../models.h"
#include "../../props.h"
#include "../../undo/variableschangecommand.h"
#include "../properties/curvepropwidget.h"
#include "../properties/propwidgets.h"
#include "../themedicon.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>

VariablesWidget::VariablesWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("VariablesPanel");

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // --- preset bar ---
    presetBar = new QWidget(this);
    presetBar->setObjectName("VariablePresetBar");
    auto bar = new QHBoxLayout(presetBar);
    bar->setContentsMargins(8, 6, 8, 6);
    bar->setSpacing(6);
    bar->addWidget(new QLabel("Preset"));
    presetCombo = new QComboBox();
    presetCombo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    bar->addWidget(presetCombo, 1);
    presetModified = new QLabel("•");
    presetModified->setObjectName("VariablePresetModified");
    presetModified->setToolTip("The values differ from the selected preset.");
    bar->addWidget(presetModified);
    presetSave = new QPushButton("Save");
    presetSave->setProperty("size", "small");
    bar->addWidget(presetSave);
    presetMenuButton = new QToolButton();
    presetMenuButton->setText("⋯");
    presetMenuButton->setPopupMode(QToolButton::InstantPopup);
    auto menu = new QMenu(presetMenuButton);
    menu->addAction("Save As New Preset…", this, [this]() { savePresetAs(); });
    auto renameAction = menu->addAction("Rename Preset…", this, [this]() { renamePreset(); });
    auto deleteAction = menu->addAction("Delete Preset", this, [this]() { deletePreset(); });
    menu->addSeparator();
    auto revertAction = menu->addAction("Revert to Preset", this, [this]() {
        applyPreset(project ? project->activePresetId : QString());
    });
    connect(menu, &QMenu::aboutToShow, this, [=]() {
        bool hasPreset = project && project->presetById(project->activePresetId);
        renameAction->setEnabled(hasPreset);
        deleteAction->setEnabled(hasPreset);
        revertAction->setText(hasPreset ? "Revert to Preset" : "Revert to Saved Values");
    });
    presetMenuButton->setMenu(menu);
    bar->addWidget(presetMenuButton);
    layout->addWidget(presetBar);

    connect(presetCombo, &QComboBox::activated, this, [this](int index) {
        applyPreset(presetCombo->itemData(index).toString());
    });
    connect(presetSave, &QPushButton::clicked, this, [this]() { savePreset(); });

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
    // Rows always fit the dock's width.
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rowsHost = new QWidget();
    rowsLayout = new QVBoxLayout(rowsHost);
    rowsLayout->setContentsMargins(0, 0, 0, 0);
    rowsLayout->setSpacing(0);
    scroll->setWidget(rowsHost);
    layout->addWidget(scroll, 1);

    refresh();
}

void VariablesWidget::setProject(const TextureProjectPtr& project)
{
    this->project = project;
    settingsOpen.clear();
    scrollToId.clear();
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
    for (const auto& preset : project->variablePresets)
        sig += preset.id + '|' + preset.name + ';';
    return sig + project->activePresetId;
}

void VariablesWidget::refreshIfChanged()
{
    if (signature() != builtSignature)
        refresh();
}

void VariablesWidget::selectVariable(const QString& id)
{
    settingsOpen.insert(id);
    scrollToId = id;
    refresh();
}

void VariablesWidget::refresh()
{
    builtSignature = signature();
    int scrollPos = scroll->verticalScrollBar()->value();

    while (rowsLayout->count() > 0) {
        auto item = rowsLayout->takeAt(0);
        if (auto w = item->widget()) {
            w->hide();
            w->deleteLater();
        }
        delete item;
    }

    bool any = project && !project->variables.isEmpty();
    emptyLabel->setVisible(!any);
    scroll->setVisible(any);
    presetBar->setVisible(any);
    if (!any)
        return;

    rebuildPresetBar();

    // Forget rows whose variable is gone.
    QSet<QString> ids;
    for (const auto& var : project->variables)
        ids.insert(var->id);
    settingsOpen.intersect(ids);

    QWidget* target = nullptr;
    for (int i = 0; i < project->variables.size(); i++) {
        auto var = project->variables[i];
        auto row = createRow(var, i);
        rowsLayout->addWidget(row);
        if (var->id == scrollToId)
            target = row;
    }
    rowsLayout->addStretch(1);

    // After layout: restore the scroll position, or bring the requested row
    // into view.
    QTimer::singleShot(0, this, [this, scrollPos, target]() {
        if (target)
            scroll->ensureWidgetVisible(target, 0, 0);
        else
            scroll->verticalScrollBar()->setValue(scrollPos);
    });
    scrollToId.clear();
}

QWidget* VariablesWidget::createRow(const TextureVariablePtr& var, int index)
{
    const QString id = var->id;
    auto row = new QWidget();
    row->setObjectName("VariableRow");
    row->setAttribute(Qt::WA_StyledBackground);
    auto layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // The value editor, with the settings toggle overlaid at the right end
    // of its label row (both in one grid cell, like the property panel's
    // link button).
    auto top = new QWidget();
    auto grid = new QGridLayout(top);
    grid->setContentsMargins(0, 0, 0, 0);
    if (auto editor = createValueEditor(var)) {
        // The widget points at editorProp; free them together.
        Prop* owned = editorProp;
        editorProp = nullptr;
        connect(editor, &QObject::destroyed, [owned]() { delete owned; });
        grid->addWidget(editor, 0, 0);
    }

    auto gear = new QToolButton(top);
    gear->setObjectName("VariableSettingsButton");
    gear->setIcon(themedToolIcon(":/icons/settings.svg"));
    gear->setIconSize(QSize(12, 12));
    gear->setCheckable(true);
    gear->setChecked(settingsOpen.contains(id));
    gear->setToolTip("Variable settings: name, bounds, options, links");
    grid->addWidget(gear, 0, 0, Qt::AlignTop | Qt::AlignRight);
    layout->addWidget(top);

    auto settings = createSettings(var, index);
    settings->setVisible(gear->isChecked());
    layout->addWidget(settings);

    connect(gear, &QToolButton::toggled, this, [this, id, settings](bool open) {
        settings->setVisible(open);
        if (open)
            settingsOpen.insert(id);
        else
            settingsOpen.remove(id);
    });
    return row;
}

QWidget* VariablesWidget::createSettings(const TextureVariablePtr& var, int index)
{
    const QString id = var->id;
    auto body = new QWidget();
    body->setObjectName("VariableSettings");
    body->setAttribute(Qt::WA_StyledBackground);
    auto layout = new QVBoxLayout(body);
    layout->setContentsMargins(10, 6, 12, 10);

    // --- names ---
    auto names = new QWidget();
    auto form = new QFormLayout(names);
    form->setContentsMargins(0, 0, 0, 0);

    auto displayEdit = new QLineEdit(var->displayName());
    form->addRow("Display Name", displayEdit);

    auto nameEdit = new QLineEdit(var->name());
    nameEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[A-Za-z_][A-Za-z0-9_]*"), nameEdit));
    nameEdit->setToolTip("Identifier used by the web runtime and API. Letters, "
                         "digits and underscores.");
    form->addRow("Name", nameEdit);
    form->addRow("Type", new QLabel(TextureVariable::typeLabel(var->type)));
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

    return body;
}

// ---------------------------------------------------------------------------
// Presets

void VariablesWidget::rebuildPresetBar()
{
    QSignalBlocker block(presetCombo);
    presetCombo->clear();
    // "Saved values": what the file was loaded/saved with.
    presetCombo->addItem("Saved values", QString());
    presetCombo->setItemData(0, "The values the file was last opened or saved with",
                             Qt::ToolTipRole);
    int current = 0;
    for (const auto& preset : project->variablePresets) {
        presetCombo->addItem(preset.name, preset.id);
        if (preset.id == project->activePresetId)
            current = presetCombo->count() - 1;
    }
    presetCombo->setCurrentIndex(current);
    updatePresetModified();
}

void VariablesWidget::updatePresetModified()
{
    if (!project)
        return;
    auto preset = project->presetById(project->activePresetId);
    const auto& values = preset ? preset->values : project->savedVariableValues;
    bool modified = !variables::valuesMatch(project->variables, values);
    // Keep the label's space so the bar doesn't jump.
    presetModified->setText(modified ? "•" : " ");
    presetSave->setToolTip(preset ? QString("Overwrite \"%1\" with the current values").arg(preset->name)
                                  : "Save the current values as a new preset");
}

void VariablesWidget::applyPreset(const QString& presetId)
{
    if (!project)
        return;
    auto preset = project->presetById(presetId);
    QString name = preset ? preset->name : "Saved values";
    auto values = preset ? preset->values : project->savedVariableValues;

    VariablesEdit edit(project, renderer, QString("Apply Preset %1").arg(name));
    for (const auto& var : project->variables)
        edit.touchVariable(var);
    for (const auto& var : variables::applyValues(project->variables, values))
        project->applyVariable(var);
    project->activePresetId = preset ? preset->id : QString();
    if (undoStack)
        undoStack->push(edit.finish());

    QTimer::singleShot(0, this, [this]() { refresh(); });
    emit valueChanged();
    emit variablesChanged();
}

void VariablesWidget::savePreset()
{
    if (!project)
        return;
    auto preset = project->presetById(project->activePresetId);
    if (!preset) {
        savePresetAs();
        return;
    }
    VariablesEdit edit(project, renderer, QString("Save Preset %1").arg(preset->name));
    preset->values = variables::captureValues(project->variables);
    if (undoStack)
        undoStack->push(edit.finish());
    updatePresetModified();
}

void VariablesWidget::savePresetAs()
{
    if (!project)
        return;
    bool ok = false;
    auto name = QInputDialog::getText(
                    this, "Save Preset", "Preset name:", QLineEdit::Normal,
                    QString("Preset %1").arg(project->variablePresets.size() + 1), &ok)
                    .trimmed();
    if (!ok || name.isEmpty())
        return;

    VariablesEdit edit(project, renderer, QString("Save Preset %1").arg(name));
    VariablePreset preset;
    preset.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    preset.name = name;
    preset.values = variables::captureValues(project->variables);
    project->variablePresets.append(preset);
    project->activePresetId = preset.id;
    if (undoStack)
        undoStack->push(edit.finish());
    builtSignature = signature();
    rebuildPresetBar();
}

void VariablesWidget::renamePreset()
{
    auto preset = project ? project->presetById(project->activePresetId) : nullptr;
    if (!preset)
        return;
    bool ok = false;
    auto name = QInputDialog::getText(this, "Rename Preset", "Preset name:",
                                      QLineEdit::Normal, preset->name, &ok)
                    .trimmed();
    if (!ok || name.isEmpty() || name == preset->name)
        return;

    VariablesEdit edit(project, renderer, QString("Rename Preset %1").arg(name));
    preset->name = name;
    if (undoStack)
        undoStack->push(edit.finish());
    builtSignature = signature();
    rebuildPresetBar();
}

void VariablesWidget::deletePreset()
{
    auto preset = project ? project->presetById(project->activePresetId) : nullptr;
    if (!preset)
        return;
    QString id = preset->id;
    VariablesEdit edit(project, renderer, QString("Delete Preset %1").arg(preset->name));
    project->variablePresets.erase(
        std::remove_if(project->variablePresets.begin(), project->variablePresets.end(),
                       [&](const VariablePreset& p) { return p.id == id; }),
        project->variablePresets.end());
    project->activePresetId.clear();
    if (undoStack)
        undoStack->push(edit.finish());
    builtSignature = signature();
    rebuildPresetBar();
}

// ---------------------------------------------------------------------------

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
    else {
        builtSignature = signature();
        updatePresetModified();
    }
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
