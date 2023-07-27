#ifndef ANIMHOSTINPUTNODE_H
#define ANIMHOSTINPUTNODE_H

#define NODE_EDITOR_SHARED 1

#include <QtNodes/NodeDelegateModel>
#include "plugininterface.h"
#include "commondatatypes.h"
#include "nodedatatypes.h"
#include "animhostnode.h"
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class ANIMHOSTCORESHARED_EXPORT AnimHostInputNode : public AnimHostNode
{
    Q_OBJECT

public:
    AnimHostInputNode() {};
    AnimHostInputNode(std::shared_ptr<PluginInterface> plugin) : AnimHostNode(plugin) {};


    std::shared_ptr<NodeData> outData(PortIndex index) override { return nullptr; };
    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override {};

    void compute() override {};

};

#endif // ANIMHOSTNODE_H
