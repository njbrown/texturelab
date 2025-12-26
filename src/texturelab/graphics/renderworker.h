#pragma once

#include <QMutex>
#include <QQueue>
#include <atomic>
#include <QObject>
#include <QList>
#include "../props.h"


struct RenderNodeInput {
    QString nodeId;
    QString inputName;
    GLuint textureId;
};

struct RenderProp {
    QString propName;
    PropType propType;
    QVariant value;
};

struct RenderCommand {
    QString nodeId;
    GLuint shaderId;

    // all expected inputs need to be cleared
    int totalInputs;
    QList<RenderNodeInput> input;

    // props
    QList<RenderProp> props;

};

class RenderWorker : public QObject {
    Q_OBJECT
    
    QMutex mutex;
    std::atomic<bool> running;
    QQueue<RenderCommand> renderQueue;
public:
    void setRenderQueue(QQueue<RenderCommand> renderQueue);
    void run();
    void kill();
    void processRenderCommand(const RenderCommand& command);
private:
    void setup();
    
signals:
    void nodeRendered(QString nodeId, GLuint textureId);
};