#ifndef ANIMHOST_H
#define ANIMHOST_H

#include <animhostcore_global.h>

#include "plugininterface.h"
#include "pluginnodeinterface.h"

#include <QObject>
#include <QFileInfo>
#include <QDirIterator>
#include <QMultiMap>

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


class ANIMHOSTCORESHARED_EXPORT AnimHost : public QObject
{
    Q_OBJECT

public:
    //functions
    AnimHost();
    void registerPlugin(std::shared_ptr<PluginInterface> plugin);
    bool loadPlugins();

    void createNodeFromNodePlugin(std::shared_ptr<PluginNodeInterface> plugin);

    //variables
    std::shared_ptr<NodeDelegateModelRegistry> nodes;

private:
    //variables
    QMultiMap<QString, std::shared_ptr<PluginInterface>> plugins;
    void createNodeFromPlugin(std::shared_ptr<PluginInterface> plugin);

protected:
};

#endif // ANALYSER_H
