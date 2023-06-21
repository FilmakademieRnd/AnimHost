#ifndef ANIMHOST_H
#define ANIMHOST_H

#include <QObject>
#include <QFileInfo>
#include <QDirIterator>
#include "plugininterface.h"
#include <QMultiMap>

#define NODE_EDITOR_SHARED 1

#include <QtNodes/ConnectionStyle>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeData>
#include <QtNodes/NodeDelegateModelRegistry>

using QtNodes::ConnectionStyle;
using QtNodes::DataFlowGraphicsScene;
using QtNodes::DataFlowGraphModel;
using QtNodes::GraphicsView;
using QtNodes::NodeDelegateModelRegistry;

class AnimHost : public QObject
{
    Q_OBJECT

public:
    //functions
    AnimHost();
    void registerPlugin(PluginInterface* plugin, QList<QVariant> inputs, QList<QVariant>* outputs);
    bool loadPlugins();

    //variables
    std::shared_ptr<NodeDelegateModelRegistry> nodes;

private:
    //variables
    QMultiMap<QString, PluginInterface*> plugins;
    void createNodeFromPlugin(PluginInterface *plugin);


    //functions


signals:

public slots:

protected:
};

#endif // ANALYSER_H
