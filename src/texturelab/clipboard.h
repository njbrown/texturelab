#pragma once

#include "models.h"
#include <QList>

class Clipboard {
public:
    static void copyItems(TextureProjectPtr project,
                          const QList<QString>& nodeIds,
                          const QList<QString>& frameIds,
                          const QList<QString>& commentIds);

    static bool pasteItems(TextureProjectPtr project,
                           QList<TextureNodePtr>& outNodes,
                           QList<ConnectionPtr>& outConnections,
                           QList<CommentPtr>& outComments,
                           QList<FramePtr>& outFrames);

    static bool hasData();

private:
    static const QString PREFIX;
};
