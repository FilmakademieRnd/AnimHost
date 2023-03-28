#ifndef ANIMHOST_H
#define ANIMHOST_H

#include <QObject>
#include <QFileInfo>
#include <QDirIterator>
#include "plugininterface.h"
#include <QMultiMap>

class AnimHost : public QObject
{
    Q_OBJECT

public:
    //functions
    AnimHost(QString filePath);
    void run();
    void registerPlugin(PluginInterface* plugin, QList<QVariant> inputs, QList<QVariant>* outputs);
    bool loadPlugins();

private:
    //variables
    QFileInfo basePath;
    QMultiMap<QString, PluginInterface*> plugins;

    //functions


signals:

public slots:

};

#endif // ANALYSER_H
