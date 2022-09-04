#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QMap>
#include <QList>
#include <QVector2D>

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

class Texture
{
public:
    int randomSeed;

    QMap<QString, TextureNode *> nodes;
    QMap<QString, Connection *> connections;
    QMap<QString, Comment *> comments;
    QMap<QString, Frame *> frames;
};

class TextureNode
{
public:
    QString id;
    QVector2D pos;
};

class Comment
{
public:
    QString id;
    QString text;
    QVector2D pos;
};

class Frame
{
public:
    QString id;
    QString text;

    QVector2D pos;
    QVector2D size;
};

class Connection
{
public:
    QString id;

    QString leftItemId;
    QString rightItemId;
};

#endif