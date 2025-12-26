#include "renderworker.h"


void RenderWorker::setRenderQueue(QQueue<RenderCommand> renderQueue){
    QMutexLocker locker(&mutex);
    this->renderQueue = renderQueue;

}

void RenderWorker::run(){
    this->setup();
    
    while(running){
        mutex.lock();
        if(!renderQueue.isEmpty()){
            RenderCommand command = renderQueue.dequeue();
            mutex.unlock();
            this->processRenderCommand(command);
        } else {
            mutex.unlock();
            // QThread::msleep(10);
        }
    }
}

void RenderWorker::setup(){
    running = true;
    // Initialize OpenGL context or other necessary setups here
}

void RenderWorker::processRenderCommand(const RenderCommand& command){
    // Here you would bind the shader, set up inputs and props, and render to a texture.
    // This is a placeholder implementation.

    // Simulate rendering process
    GLuint renderedTextureId = 0; // Replace with actual texture ID after rendering

    // Emit signal that node has been rendered
    emit onNodeRendered(command.nodeId, renderedTextureId);
}

void RenderWorker::kill(){
    running = false;
}
    