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

class Project
{
public:
};

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

    QMap<QString, TextureNode *> nodes;
    QMap<QString, Connection *> connections;
    QMap<QString, Comment *> comments;
    QMap<QString, Frame *> frames;
};

class TextureNode : public QEnableSharedFromThis<TextureNode>
{
public:
    QString id;
    QVector2D pos;

    long randomSeed;
    QString exportName;

    QList<Prop *> props;

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

    QString leftItemId;
    QString rightItemId;

    QString leftName;
    QString rightName;
};

#endif