#ifndef ANIMHOSTNODE_H
#define ANIMHOSTNODE_H

#define NODE_EDITOR_SHARED 1

#include <QtNodes/NodeDelegateModel>
#include "plugininterface.h"
#include "commondatatypes.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class AnimHostNode : public NodeDelegateModel
{
public:
    AnimHostNode(PluginInterface plugin);

    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;
    QWidget *embeddedWidget() override { return nullptr; }

protected:
     void compute();

protected:
    //std::weak_ptr<HumanoidBonesData> _number1;
    //std::weak_ptr<FloatData> _number2;
    //std::shared_ptr<HumanoidBonesData> _result;

private:
    PluginInterface* _plugin;

    NodeDataType convertQMetaTypeToNodeDataType(QMetaType qType) const;
};

#endif // ANIMHOSTNODE_H
