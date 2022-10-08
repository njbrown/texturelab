#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QMap>
#include <QList>
#include <QVector2D>
#include <QSharedPointer>
#include <QEnableSharedFromThis>

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

enum class PackageFileType
{
    Texture,
    Image
};

class ProjectFile
{
public:
    QByteArray contents();
};

class TextureProject : public QEnableSharedFromThis<TextureProject>
{
public:
    int randomSeed;
    Library *library;

    QMap<QString, TextureNodePtr> nodes;
    QMap<QString, ConnectionPtr> connections;
    QMap<QString, CommentPtr> comments;
    QMap<QString, FramePtr> frames;

    TextureNodePtr getNodeById(const QString &id);

    // todo: make two port variant
    void addConnection(TextureNodePtr leftNode, TextureNodePtr rightNode, QString rightNodeInput);
};

class TextureNode : public QEnableSharedFromThis<TextureNode>
{
public:
    QString id;
    QVector2D pos;

    long randomSeed;
    QString exportName;

    QMap<QString, Prop *> props;

    void setProp(QString propName, QVariant value);
};

class Comment : public QEnableSharedFromThis<Comment>
{
public:
    QString id;
    QString text;
    QVector2D pos;
};

class Frame : public QEnableSharedFromThis<Frame>
{
public:
    QString id;
    QString text;

    QVector2D pos;
    QVector2D size;
};

class Connection : public QEnableSharedFromThis<Connection>
{
public:
    QString id;

    TextureNodePtr leftNode;
    TextureNodePtr rightNode;

    QString leftNodeOutputName;
    QString rightNodeInputName;
};

#endif