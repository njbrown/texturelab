#ifndef MODELS_H
#define MODELS_H

#include <QEnableSharedFromThis>
#include <QList>
#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QVector2D>

class QOpenGLFramebufferObject;
class QOpenGLShaderProgram;

class TextureProject;
class TextureNode;
class Comment;
class Frame;
class Connection;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureNode> TextureNodePtr;
typedef QSharedPointer<Comment> CommentPtr;
typedef QSharedPointer<Frame> FramePtr;
typedef QSharedPointer<Connection> ConnectionPtr;

class Prop;
class Library;

class IntProp;
class FloatProp;
class BoolProp;
class EnumProp;
class ColorProp;
class StringProp;
class ImageProp;

enum class PackageFileType { Texture, Image };

class ProjectFile {
public:
    QByteArray contents();
};

class TextureProject : public QEnableSharedFromThis<TextureProject> {
public:
    int randomSeed;
    int textureWidth = 1024;
    int textureHeight = 1024;

    Library* library = nullptr;

    QMap<QString, TextureNodePtr> nodes;
    QMap<QString, ConnectionPtr> connections;
    QMap<QString, CommentPtr> comments;
    QMap<QString, FramePtr> frames;

    TextureNodePtr getNodeById(const QString& id);
    ConnectionPtr getConnectionById(const QString& id);
    QVector<TextureNodePtr> getNodeDependencies(const QString& id);

    void addNode(const TextureNodePtr& node);

    // todo: make two port variant
    void addConnection(TextureNodePtr leftNode, TextureNodePtr rightNode,
                       QString rightNodeInput);

    ConnectionPtr removeConnection(const QString& leftNode,
                                   const QString& rightNode,
                                   const QString& rightNodeInput);
    void removeConnection(ConnectionPtr con);
    void removeConnection(const QString& id);

    static TextureProjectPtr createEmpty(Library* library = nullptr);
};

class TextureNode : public QEnableSharedFromThis<TextureNode> {
public:
    QString id;
    QString title;

    QVector2D pos;

    QList<QString> inputs;

    long randomSeed;
    QString exportName;

    QMap<QString, Prop*> props;

    // texture needs updating
    bool isDirty;

    int textureWidth;
    int textureHeight;
    QOpenGLFramebufferObject* texture = nullptr;
    QOpenGLShaderProgram* shader = nullptr;
    QString shaderSource;

    TextureNode();

    virtual void init(){};

    void addInput(const QString& inputName);

    void setProp(QString propName, QVariant value);

    void setShaderSource(const QString& source) { shaderSource = source; }

    bool isGraphicsResourcesInitialized()
    {
        return texture != nullptr && shader != nullptr;
    }

    // add prop functions
    IntProp* addIntProp(const QString& name, const QString& displayName,
                        int defaultVal = 1, int minVal = 1, int maxVal = 100,
                        int increment = 1);
    FloatProp* addFloatProp(const QString& name, const QString& displayName,
                            double defaultVal = 1, double minVal = 1,
                            double maxVal = 100, double increment = 1);
    BoolProp* addBoolProp(const QString& name, const QString& displayName,
                          bool defaultVal = false);
    EnumProp* addEnumProp(const QString& name, const QString& displayName,
                          QList<QString> defaultVal);
    ColorProp* addColorProp(const QString& name, const QString& displayName,
                            const QColor& defaultVal);
    StringProp* addStringProp(const QString& name, const QString& displayName,
                              const QString& defaultVal = "");

    // GradientProp* addGradientProp(const QString& id, const QString&
    // displayName,
    //                               Gradient defaultVal);

    ImageProp* addImageProp(const QString& name, const QString& displayName);
};

class Comment : public QEnableSharedFromThis<Comment> {
public:
    QString id;
    QString text;
    QVector2D pos;
};

class Frame : public QEnableSharedFromThis<Frame> {
public:
    QString id;
    QString text;

    QVector2D pos;
    QVector2D size;
};

class Connection : public QEnableSharedFromThis<Connection> {
public:
    QString id;

    TextureNodePtr leftNode;
    TextureNodePtr rightNode;

    QString leftNodeOutputName;
    QString rightNodeInputName;
};

#endif