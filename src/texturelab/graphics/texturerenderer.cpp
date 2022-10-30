#include "texturerenderer.h"
// #include "../models.h"

// https://forum.qt.io/topic/84779/how-to-create-a-qoffscreensurface-correctly/4
// #include <QGLFramebufferObject>
#include <QImage>
#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
// Qt6 only!!
#include <QOpenGLVersionFunctionsFactory>
// #include <QOpenGLPaintDevice>
// #include <QtGui/QOpenGLFunctions_3_3_Core>

#include "models.h"

enum class VertexUsage : int {
    Position = 0,
    Color = 1,
    TexCoord0 = 2,
    TexCoord1 = 3,
    TexCoord2 = 4,
    TexCoord3 = 5,
    Normal = 6,
    Tangent = 7,
    Count = 8
};

const int TEXTURE_SIZE = 1024;

// https://github.com/cromop/mOffscreenRendering/blob/master/OGLWidget.cpp
// https://github.com/florianblume/Qt3D-OffscreenRenderer/blob/master/offscreensurfaceframegraph.h
// https://stackoverflow.com/questions/60515589/offscreen-render-with-qoffscreensurface-using-docker
void TextureRenderer::testRendering()
{
    // create shader
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex);
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment);
    auto program = new QOpenGLShaderProgram;

    QString vSource = R""""(
        #version 150 core

        in vec3 a_pos;
        in vec2 a_texCoord;

        out vec2 v_texCoord;

        void main()
        {
                v_texCoord = a_texCoord*vec2(1,1);
                gl_Position = vec4(a_pos,1);
        }
    )"""";

    QString fSource = R""""(
        #version 150 core

        in vec2 v_texCoord;

        out vec4 fragColor;

        void main()
        {
            vec4 col = vec4(v_texCoord.x, v_texCoord.y, 1.0, 1.0);

            fragColor = col;
        }
    )"""";

    if (!vshader->compileSourceCode(vSource)) {
        qDebug() << "VERTEX SHADER ERROR";
        qDebug() << vshader->log();
    }

    if (!fshader->compileSourceCode(fSource)) {
        qDebug() << "FRAGMENT SHADER ERROR";
        qDebug() << fshader->log();
    }

    program->removeAllShaders();

    program->addShader(vshader);
    program->addShader(fshader);

    program->bindAttributeLocation("a_pos", (int)VertexUsage::Position);
    program->bindAttributeLocation("a_color", (int)VertexUsage::Color);
    program->bindAttributeLocation("a_texCoord", (int)VertexUsage::TexCoord0);

    if (!program->link()) {
        qDebug() << "SHADER LINK ERROR";
        qDebug() << program->log();
    }

    // render quad (bind vbo, shader, vbo, etc)
    fbo->bind();
    gl->glViewport(0, 0, TEXTURE_SIZE, TEXTURE_SIZE);

    gl->glClearColor(0, 0, 0, 1);
    gl->glClearDepth(0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vao->bind();

    program->bind();

    vbo->bind();
    gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
    gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT, GL_FALSE,
                              5 * sizeof(float), nullptr);
    gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                              GL_FALSE, 5 * sizeof(float),
                              reinterpret_cast<void*>(3 * sizeof(float)));

    gl->glDrawArrays(GL_TRIANGLES, 0, 6);

    vbo->release();

    gl->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());

    // grab pixels to pixmap
    auto img = fbo->toImage();
    img.save("./render.png");

    // save to desktop
}

void TextureRenderer::setup()
{
    // create surface
    surface = new QOffscreenSurface();
    // QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    // format.setMajorVersion(3);
    // format.setMinorVersion(2);

    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setMajorVersion(3);
    format.setMinorVersion(2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(1);
    format.setSwapInterval(0);
    format.setOption(QSurfaceFormat::DebugContext); // for debugging

    surface->setFormat(format);
    surface->create();

    // create context
    ctx = new QOpenGLContext();
    ctx->setFormat(format);
    if (!ctx->create()) {
        qFatal("unable to create surface!");
    }

    ctx->makeCurrent(surface);

    // https://doc-snapshots.qt.io/qt6-dev/gui-changes-qt6.html
    gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_2_Core>(ctx);
    if (!gl) {
        qFatal("Could not obtain required OpenGL context version");
    }

    // setup debugging
    QOpenGLDebugLogger* logger = new QOpenGLDebugLogger();
    logger->initialize();

    QObject::connect(logger, &QOpenGLDebugLogger::messageLogged,
                     [=](const QOpenGLDebugMessage& debugMessage) {
                         qDebug() << debugMessage;
                     });

    logger->startLogging();

    gl->initializeOpenGLFunctions();

    // simple setup
    gl->glViewport(0, 0, TEXTURE_SIZE, TEXTURE_SIZE);
    gl->glDisable(GL_BLEND);
    gl->glDisable(GL_DEPTH_TEST);
    gl->glDisable(GL_CULL_FACE);

    // setup vbo
    vao = new QOpenGLVertexArrayObject;
    if (vao->create())
        vao->bind();

    // setup vertex array
    QVector<float> data;
    // TRIANGLE 1
    data.append(-1);
    data.append(-1);
    data.append(0);

    data.append(0);
    data.append(0);

    data.append(1);
    data.append(-1);
    data.append(0);

    data.append(1);
    data.append(0);

    data.append(-1);
    data.append(1);
    data.append(0);

    data.append(0);
    data.append(1);

    // TRIANGLE 2
    data.append(-1);
    data.append(1);
    data.append(0);

    data.append(0);
    data.append(1);

    data.append(1);
    data.append(-1);
    data.append(0);

    data.append(1);
    data.append(0);

    data.append(1);
    data.append(1);
    data.append(0);

    data.append(1);
    data.append(1);

    vbo = new QOpenGLBuffer;
    vbo->create();
    vbo->bind();
    vbo->allocate(data.constData(), data.count() * sizeof(float));
    vbo->release();

    // create fbo
    // https://doc.qt.io/qt-6/qopenglframebufferobject.html
    // https://www.qt.io/blog/2015/09/21/using-modern-opengl-es-features-with-qopenglframebufferobject-in-qt-5-6
    fbo = new QOpenGLFramebufferObject(TEXTURE_SIZE, TEXTURE_SIZE);
    if (!fbo->isValid()) {
        qFatal("FBO could not be created");
    }
}

TextureRenderer::TextureRenderer() { this->setup(); }

void TextureRenderer::setProject(TextureProjectPtr project)
{
    this->project = project;
}

void TextureRenderer::update()
{
    if (!project)
        return;

    // check for nodes that need updating and update
    for (auto& node : project->nodes) {
        if (!node->isGraphicsResourcesInitialized()) {
            // create texture
            initializeNodeGraphicsResources(node);
        }

        // if the resolution has changed, resize texture
        if (project->textureWidth != node->textureWidth ||
            project->textureHeight != node->textureHeight) {
            // resize
            // resizeNodeTexture(node);
            node->textureWidth = project->textureWidth;
            node->textureHeight = project->textureHeight;
            node->texture = new QOpenGLFramebufferObject(node->textureWidth,
                                                         node->textureHeight);

            // clear pixmap and emit thumbnail changed?
        }

        // if (node->needsUpdate()) {

        //     // process
        // }
    }

    ctx->makeCurrent(surface);
    // todo: use quota
    while (true) {
        auto nextNode = getNextUpdatableNode();
        if (!nextNode)
            break;

        qDebug() << "Rendering node: " << nextNode->id;
        renderNode(nextNode);

        nextNode->isDirty = false;

        auto img = nextNode->texture->toImage();
        emit thumbnailGenerated(nextNode->id, QPixmap::fromImage(img));
    }

    ctx->doneCurrent();
}

void TextureRenderer::initializeNodeGraphicsResources(
    const TextureNodePtr& node)
{
    ctx->makeCurrent(surface);

    // create fbo
    node->texture = new QOpenGLFramebufferObject(project->textureWidth,
                                                 project->textureWidth);
    node->textureWidth = project->textureWidth;
    node->textureHeight = project->textureHeight;

    if (!node->texture->isValid()) {
        qFatal("FBO could not be created");
    }

    // build and compile shaders
    node->shader = buildShaderForNode(node);
    ctx->doneCurrent();
}

void TextureRenderer::renderNode(const TextureNodePtr& node)
{
    node->texture->bind();
    gl->glViewport(0, 0, node->textureWidth, node->textureHeight);

    gl->glClearColor(0, 0, 0, 1);
    gl->glClearDepth(0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vao->bind();

    if (node->shader->isLinked()) {
        node->shader->bind();

        // clear all inputs
        int texIndex = 0;
        for (auto input : node->inputs) {
            gl->glActiveTexture(GL_TEXTURE0 + texIndex);
            gl->glBindTexture(GL_TEXTURE_2D, 0);

            // gl->glUniform1i(node->shader->uniformLocation(input), 0);
            node->shader->setUniformValue(input.toStdString().c_str(), 0);
            node->shader->setUniformValue(
                (input + "_connected").toStdString().c_str(), 0);

            texIndex++;
        }

        // pass inputs
        auto nodeInputs = getNodeInputs(node);
        texIndex = 0;
        for (auto nodeInput : nodeInputs) {
            gl->glActiveTexture(GL_TEXTURE0 + texIndex);
            // if (!nodeInput.node->texture->bind())
            //     qFatal("could not bind texture");
            gl->glBindTexture(GL_TEXTURE_2D,
                              nodeInput.node->texture->texture());

            auto name = nodeInput.name;
            // gl->glUniform1i(node->shader->uniformLocation(input), 0);
            node->shader->setUniformValue(name.toStdString().c_str(), texIndex);
            node->shader->setUniformValue(
                (name + "_connected").toStdString().c_str(), 1);

            texIndex++;
        }

        // render triangles
        vbo->bind();
        gl->glEnableVertexAttribArray((int)VertexUsage::Position);
        gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
        gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT,
                                  GL_FALSE, 5 * sizeof(float), nullptr);
        gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                                  GL_FALSE, 5 * sizeof(float),
                                  reinterpret_cast<void*>(3 * sizeof(float)));

        gl->glDrawArrays(GL_TRIANGLES, 0, 6);

        vbo->release();
    }

    vao->release();

    // gl->glBindFramebuffer(GL_FRAMEBUFFER,
    // ctx->defaultFramebufferObject());

    // grab pixels to pixmap
    // auto img = node->texture->toImage();
    // img.save(node->id + ".png");

    node->texture->release();
}

QVector<NodeInput> TextureRenderer::getNodeInputs(const TextureNodePtr& node)
{
    QVector<NodeInput> inputs;
    for (auto con : project->connections) {
        if (con->rightNode == node) {
            inputs.append({con->leftNode, con->rightNodeInputName});
        }
    }

    return inputs;
}

TextureNodePtr TextureRenderer::getNextUpdatableNode() const
{
    // for each node, if node is dirty and all deps are
    // non-dirty the this is a valid node

    for (auto node : project->nodes) {
        if (!node->isDirty)
            continue;

        auto hasCleanDeps = true;

        // we have a dirty node, check if all deps are clean
        auto deps = project->getNodeDependencies(node->id);
        for (auto dep : deps) {
            if (dep->isDirty) {
                hasCleanDeps = false;
                break;
            }
        }

        if (hasCleanDeps) {
            return node;
        }
    }

    return TextureNodePtr(nullptr);
}

QOpenGLShaderProgram*
TextureRenderer::buildShaderForNode(const TextureNodePtr& node)
{
    ctx->makeCurrent(surface);

    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex);
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment);
    auto program = new QOpenGLShaderProgram;

    QString vSource = R""""(
        #version 150 core

        //precision highp float;

        in vec3 a_pos;
        in vec2 a_texCoord;

        out vec2 v_texCoord;

        void main()
        {
                v_texCoord = a_texCoord;
                gl_Position = vec4(a_pos,1);
        }
    )"""";

    QString fSource = R""""(
        #version 150 core
        //precision highp float;
        in vec2 v_texCoord;

        #define GRADIENT_MAX_POINTS 32        

        vec4 process(vec2 uv);
        void initRandom();

        uniform vec2 _textureSize;

        out vec4 fragColor;
            
        void main() {
            initRandom();
			vec4 result = process(v_texCoord);
			fragColor = clamp(result, 0.0, 1.0);
        }
        
    )"""";

    fSource = fSource + this->createRandomLib() + this->createGradientLib() +
              this->createCodeForInputs(node) + this->createCodeForProps(node) +
              "#line 0\n" + node->shaderSource;

    if (!vshader->compileSourceCode(vSource)) {
        qDebug() << "VERTEX SHADER ERROR";
        qDebug() << vshader->log();
    }

    if (!fshader->compileSourceCode(fSource)) {
        qDebug() << "FRAGMENT SHADER ERROR";
        qDebug() << fshader->log();
    }

    program->removeAllShaders();

    program->addShader(vshader);
    program->addShader(fshader);

    program->bindAttributeLocation("a_pos", (int)VertexUsage::Position);
    program->bindAttributeLocation("a_color", (int)VertexUsage::Color);
    program->bindAttributeLocation("a_texCoord", (int)VertexUsage::TexCoord0);

    if (!program->link()) {
        qDebug() << "SHADER LINK ERROR";
        qDebug() << program->log();
    }

    ctx->doneCurrent();

    return program;
}

QString TextureRenderer::createRandomLib()
{
    return R""""(
        // this offsets the random start (should be a uniform)
        uniform float _seed;
        // this is the starting number for the rng
        // (should be set from the uv coordinates so it's unique per pixel)
        vec2 _randomStart;

        // gives a much better distribution at 1
        #define RANDOM_ITERATIONS 1

        #define HASHSCALE1 443.8975
        #define HASHSCALE3 vec3(443.897, 441.423, 437.195)
        #define HASHSCALE4 vec4(443.897, 441.423, 437.195, 444.129)

        //  1 out, 2 in...
        float hash12(vec2 p)
        {
            vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
            p3 += dot(p3, p3.yzx + 19.19);
            return fract((p3.x + p3.y) * p3.z);
        }

        ///  2 out, 2 in...
        vec2 hash22(vec2 p)
        {
            vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
            p3 += dot(p3, p3.yzx+19.19);
            return fract((p3.xx+p3.yz)*p3.zy);

        }


        float _rand(vec2 uv)
        {
            float a = 0.0;
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                // 0.005 is a good value
                vec2 pos = (uv * v);
                a += hash12(pos);
            }

            return a/float(RANDOM_ITERATIONS);
        }

        vec2 _rand2(vec2 uv)
        {
            vec2 a = vec2(0.0);
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                // 0.005 is a good value
                vec2 pos = (uv * v);
                a += hash22(pos);
            }

            return a/float(RANDOM_ITERATIONS);
        }

        float randomFloat(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomVec2(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomFloat(int index, float start, float end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + r*(end-start);
        }

        int randomInt(int index, int start, int end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + int(r*float(end-start));
        }

        bool randomBool(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index)) > 0.5;
        }

        void initRandom()
        {
            _randomStart = v_texCoord;
        }
        )"""";
}

QString TextureRenderer::createGradientLib()
{
    return R""""(
        struct Gradient {
				vec3 colors[GRADIENT_MAX_POINTS];
				float positions[GRADIENT_MAX_POINTS];
				int numPoints;
    };
        
    // assumes points are sorted
    vec3 sampleGradient(vec3 colors[GRADIENT_MAX_POINTS], float positions[GRADIENT_MAX_POINTS], int numPoints, float t)
    {
        if (numPoints == 0)
            return vec3(1,0,0);
        
        if (numPoints == 1)
            return colors[0];
        
        // here at least two points are available
        if (t <= positions[0])
            return colors[0];
        
        int last = numPoints - 1;
        if (t >= positions[last])
            return colors[last];
        
        // find two points in-between and lerp
        
        for(int i = 0; i < numPoints-1;i++) {
            if (positions[i+1] > t) {
                vec3 colorA = colors[i];
                vec3 colorB = colors[i+1];
                
                float t1 = positions[i];
                float t2 = positions[i+1];
                
                float lerpPos = (t - t1)/(t2 - t1);
                return mix(colorA, colorB, lerpPos);
                
            }
            
        }
        
        return vec3(0,0,0);
    }

    vec3 sampleGradient(Gradient gradient, float t)
    {
      return sampleGradient(gradient.colors, gradient.positions, gradient.numPoints, t);
    }
        )"""";
}
QString TextureRenderer::createCodeForInputs(const TextureNodePtr& node)
{
    QString code = "";
    for (auto input : node->inputs) {
        code += "uniform sampler2D " + input + ";\n";
        code += "uniform bool " + input + "_connected;\n";
    }

    return code;
}

QString TextureRenderer::createCodeForProps(const TextureNodePtr& node)
{
    return "";
}