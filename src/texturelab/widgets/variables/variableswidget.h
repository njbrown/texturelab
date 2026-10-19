#pragma once

#include <QSet>
#include <QSharedPointer>
#include <QUndoStack>
#include <QWidget>
#include <functional>

class TextureProject;
class TextureVariable;
class TextureRenderer;
class Prop;
class QComboBox;
class QPushButton;
class QToolButton;
class QVBoxLayout;
class QLabel;
class QScrollArea;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureVariable> TextureVariablePtr;

// The "Variables" dock.
//
//   [Preset ▾] •  [Save] [⋯]        preset bar
//   Roughness  ━━━●━━━  0.45   ⚙    one row per variable: value always editable
//   Tint       [██████]        ⚙
//     ...settings...                ⚙ reveals that variable's settings (names,
//                                   bounds/options, links, move/delete)
//
// Holds variable and preset ids, never pointers: undo/redo replaces the
// variable objects.
class VariablesWidget : public QWidget {
    Q_OBJECT

public:
    VariablesWidget(QWidget* parent = nullptr);

    void setProject(const TextureProjectPtr& project);
    void setUndoStack(QUndoStack* stack) { undoStack = stack; }
    void setTextureRenderer(TextureRenderer* renderer)
    {
        this->renderer = renderer;
    }

    // Rebuild from the model, keeping open settings and the scroll position.
    void refresh();
    // refresh(), but only if variables, links or presets changed since the
    // last build — e.g. a graph edit deleted a linked node. Cheap enough to
    // call on every undo stack move.
    void refreshIfChanged();
    // Scroll to this variable and open its settings.
    void selectVariable(const QString& id);

signals:
    // A variable's value changed: linked props changed, re-render.
    void valueChanged();
    // Anything else changed (links, names, list): refresh the property panel.
    void variablesChanged();
    // Clicked a linked prop: select its node.
    void nodeActivated(const QString& nodeId);

private:
    QString signature() const;
    QWidget* createRow(const TextureVariablePtr& var, int index);
    QWidget* createSettings(const TextureVariablePtr& var, int index);
    void moveVariable(const QString& id, int delta);
    void deleteVariable(const QString& id);

    // --- presets ---
    void rebuildPresetBar();
    // Refresh just the modified marker (after a value edit).
    void updatePresetModified();
    // Apply a preset's values ("" = the saved values) as one undo step.
    void applyPreset(const QString& presetId);
    void savePreset();
    void savePresetAs();
    void renamePreset();
    void deletePreset();

    QWidget* createValueEditor(const TextureVariablePtr& var);
    QWidget* createBoundsEditor(const TextureVariablePtr& var);
    QWidget* createOptionsEditor(const TextureVariablePtr& var);
    QWidget* createLinksEditor(const TextureVariablePtr& var);

    // Runs `change` on the variable as one undo step. Value changes pass
    // mergeValue so slider scrubs merge; they don't rebuild the panel.
    void editVariable(const QString& id, const QString& text,
                      const std::function<void(TextureVariablePtr)>& change,
                      bool mergeValue = false, bool rebuild = true);

    TextureProjectPtr project;
    QUndoStack* undoStack = nullptr;
    TextureRenderer* renderer = nullptr;

    QSet<QString> settingsOpen; // variable ids whose settings are shown
    QString scrollToId;         // row to bring into view after a rebuild
    QString builtSignature;

    QWidget* presetBar;
    QComboBox* presetCombo;
    QLabel* presetModified;
    QPushButton* presetSave;
    QToolButton* presetMenuButton;
    QLabel* emptyLabel;
    QScrollArea* scroll;
    QWidget* rowsHost;
    QVBoxLayout* rowsLayout;
    // Set by createValueEditor: the copy of the variable's prop its widget
    // edits (undo replaces the variable's). Freed with that widget.
    Prop* editorProp = nullptr;
};
