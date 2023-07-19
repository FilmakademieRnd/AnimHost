#ifndef ANIMHOST_H
#define ANIMHOST_H

#include <QObject>
#include <QFileInfo>
#include <QDirIterator>
#include "plugininterface.h"
#include <QMultiMap>

#include "commondatatypes.h"

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

class ANIMHOSTCORESHARED_EXPORT AnimHost : public QObject
{
    //Q_OBJECT

public:
    //functions
    AnimHost();
    void registerPlugin(std::shared_ptr<PluginInterface> plugin);
    bool loadPlugins();

    //variables
    std::shared_ptr<NodeDelegateModelRegistry> nodes;

private:
    //variables
    QMultiMap<QString, std::shared_ptr<PluginInterface>> plugins;
    void createNodeFromPlugin(std::shared_ptr<PluginInterface> plugin);


    //functions




protected:
};

#endif // ANALYSER_H
