#ifndef ANIMHOSTNODE_H
#define ANIMHOSTNODE_H

#define NODE_EDITOR_SHARED 1

#include <QtNodes/NodeDelegateModel>
#include "plugininterface.h"
#include "commondatatypes.h"
#include "nodedatatypes.h"
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class ANIMHOSTCORESHARED_EXPORT AnimHostNode : public NodeDelegateModel
{
    Q_OBJECT

public:
    AnimHostNode(){};
    AnimHostNode(std::shared_ptr<PluginInterface> plugin);

    QString caption() const override { return this->name(); }

    bool captionVisible() const override { return true; }

    QString name() const override { return(!_plugin) ?  "NONE" : _plugin->name(); }

    unsigned int nPorts(PortType portType) const override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    virtual std::shared_ptr<NodeData> outData(PortIndex port) = 0;

    virtual void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) = 0;

    QWidget *embeddedWidget() override { return nullptr; }

protected:
     virtual void compute() = 0;

     static NodeDataType convertQMetaTypeToNodeDataType(QMetaType qType);
     static std::shared_ptr<AnimNodeDataBase> createAnimNodeDataFromID(QMetaType qType);

protected:

    std::vector<std::weak_ptr<NodeData>> _dataIn;

    std::vector<std::shared_ptr<NodeData>> _dataOut;

    std::shared_ptr<PluginInterface> _plugin;

    
};

#endif // ANIMHOSTNODE_H
