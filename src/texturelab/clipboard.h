#pragma once

#include "models.h"
#include <QList>

// A variable link on a pasted node's prop, re-established on paste only if
// the variable exists in the target project and accepts the prop.
struct PastedVariableLink {
    QString variableId;
    VariableLink link; // with the pasted node's new id
};

class Clipboard {
public:
    static void copyItems(TextureProjectPtr project,
                          const QList<QString>& nodeIds,
                          const QList<QString>& frameIds,
                          const QList<QString>& commentIds);

    static bool pasteItems(TextureProjectPtr project,
                           QPointF viewCenter,
                           QList<TextureNodePtr>& outNodes,
                           QList<ConnectionPtr>& outConnections,
                           QList<CommentPtr>& outComments,
                           QList<FramePtr>& outFrames,
                           QList<PastedVariableLink>* outLinks = nullptr);

    static bool hasData();

private:
    static const QString PREFIX;
};
