#ifndef ANIMHOSTOPERATIONNODE_H
#define ANIMHOSTOPERATIONNODE_H

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

class ANIMHOSTCORESHARED_EXPORT AnimHostOperationNode : public AnimHostNode
{
    Q_OBJECT

public:
    AnimHostOperationNode() {};
    AnimHostOperationNode(std::shared_ptr<PluginInterface> plugin) : AnimHostNode(plugin) {};

    void processInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

    bool hasInputRunSignal() const override { return true; };
    bool hasOutputRunSignal() const override { return true; };

    void compute() override;

};

#endif // ANIMHOSTNODE_H
