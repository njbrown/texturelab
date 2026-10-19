#pragma once

#include <QSharedPointer>
#include <QUndoStack>
#include <QWidget>

class TextureProject;
class TextureVariable;
class TextureRenderer;
class Prop;
class QListWidget;
class QVBoxLayout;
class QPushButton;
class QLabel;
class QScrollArea;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureVariable> TextureVariablePtr;

// The "Variables" dock: the texture's exposed variables, and an editor for the
// selected one (display name, name, value, bounds, enum options, links).
//
// Holds variable ids, never pointers: undo/redo replaces the variable objects.
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

    // Rebuild from the model, keeping the selection if it still exists.
    void refresh();
    // refresh(), but only if variables or their links changed since the last
    // build — e.g. a graph edit deleted a linked node. Cheap enough to call on
    // every undo stack move.
    void refreshIfChanged();
    void selectVariable(const QString& id);

signals:
    // A variable's value changed: linked props changed, re-render.
    void valueChanged();
    // Anything else changed (links, names, list): refresh the property panel.
    void variablesChanged();
    // Clicked a linked prop: select its node.
    void nodeActivated(const QString& nodeId);

private:
    TextureVariablePtr selected() const;
    void rebuildList();
    void rebuildEditor();
    void moveSelected(int delta);
    void deleteSelected();

    QWidget* createValueEditor(const TextureVariablePtr& var);
    QWidget* createBoundsEditor(const TextureVariablePtr& var);
    QWidget* createOptionsEditor(const TextureVariablePtr& var);
    QWidget* createLinksEditor(const TextureVariablePtr& var);

    // Runs `change` on the selected variable as one undo step. Value changes
    // pass mergeValue so slider scrubs merge.
    void editSelected(const QString& text,
                      const std::function<void(TextureVariablePtr)>& change,
                      bool mergeValue = false,
                      bool rebuild = true);

    TextureProjectPtr project;
    QUndoStack* undoStack = nullptr;
    TextureRenderer* renderer = nullptr;
    QString selectedId;
    bool rebuilding = false;
    QString builtSignature;
    QString lastBuiltId; // variable the editor was last built for
    QString signature() const;

    QListWidget* list;
    QPushButton* upButton;
    QPushButton* downButton;
    QPushButton* deleteButton;
    QLabel* emptyLabel;
    QScrollArea* scroll;
    QWidget* editorHost;
    QVBoxLayout* editorLayout;
    // The value editor works on a copy of the variable's prop (undo replaces
    // the variable's). Owned by the editor widget's lifetime.
    Prop* editorProp = nullptr;
};
