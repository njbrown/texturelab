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
class QVBoxLayout;
class QLabel;
class QScrollArea;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureVariable> TextureVariablePtr;

// The "Variables" dock: one collapsible section per exposed variable, each
// holding that variable's editor (display name, name, value, bounds, enum
// options, links, and move/delete), so editing stays next to the variable.
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

    // Rebuild from the model, keeping which sections are open and the scroll
    // position.
    void refresh();
    // refresh(), but only if variables or their links changed since the last
    // build — e.g. a graph edit deleted a linked node. Cheap enough to call on
    // every undo stack move.
    void refreshIfChanged();
    // Open this variable's section and scroll to it.
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
    QWidget* createSection(const TextureVariablePtr& var, int index);
    QString sectionTitle(const TextureVariablePtr& var) const;
    void moveVariable(const QString& id, int delta);
    void deleteVariable(const QString& id);

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

    QSet<QString> expanded; // ids of open sections
    QString scrollToId;     // section to bring into view after a rebuild
    QString builtSignature;

    QLabel* emptyLabel;
    QScrollArea* scroll;
    QWidget* sectionsHost;
    QVBoxLayout* sectionsLayout;
    // Set by createValueEditor: the copy of the variable's prop its widget
    // edits (undo replaces the variable's). Freed with that widget.
    Prop* editorProp = nullptr;
};
