#include "animhostnode.h"

AnimHostNode::AnimHostNode(std::shared_ptr<PluginInterface> plugin)
{
    _plugin = plugin;

    for (int i = 0; i < _plugin->inputs.size(); i++){
        this->_dataIn.push_back(std::weak_ptr<QtNodes::NodeData>());
    }

    for (int i = 0; i < _plugin->outputs.size(); i++) {
        this->_dataOut.push_back(std::shared_ptr<QtNodes::NodeData>());
    }
}


unsigned int AnimHostNode::nPorts(PortType portType) const
{
    unsigned int result;

    if (portType == PortType::In)
        result = _plugin->inputs.size();
    else
        result = _plugin->outputs.size();

    return result;
}


NodeDataType AnimHostNode::dataType(PortType portType, PortIndex index) const
{
    QMetaType metaType;
    if (portType == PortType::In)
        metaType = QMetaType(_plugin->inputs[index]);    
    else
        metaType = QMetaType(_plugin->outputs[index]);

    return convertQMetaTypeToNodeDataType(metaType);
}


NodeDataType AnimHostNode::convertQMetaTypeToNodeDataType(QMetaType qType)
{

    int typeId = qType.id();


    if (typeId == QMetaType::fromName("HumanoidBones").id())
        return AnimNodeData<HumanoidBones>::staticType();
    else if (typeId == QMetaType::fromName("Pose").id())
        return AnimNodeData<Pose>::staticType();
    else if (typeId == QMetaType::fromName("Skeleton").id())
        return AnimNodeData<Skeleton>::staticType();
    else if (typeId == QMetaType::fromName("Animation").id())
        return AnimNodeData<Animation>::staticType();
    else if (typeId == QMetaType::fromName("PoseSequence").id())
        return AnimNodeData<PoseSequence>::staticType();
    else
        throw "Unknown Datatype";
}


std::shared_ptr<AnimNodeDataBase> AnimHostNode::createAnimNodeDataFromID(QMetaType qType)
{
    int typeId = qType.id();

    if (typeId == QMetaType::fromName("HumanoidBones").id())
        return std::make_shared<AnimNodeData<HumanoidBones>>();
    else if (typeId == QMetaType::fromName("Pose").id())
        return std::make_shared<AnimNodeData<Pose>>();
    else if (typeId == QMetaType::fromName("shared_ptr<Pose>").id())
        return std::make_shared<AnimNodeData<Pose>>();
    else if (typeId == QMetaType::fromName("Skeleton").id())
        return std::make_shared<AnimNodeData<Skeleton>>();
    else if (typeId == QMetaType::fromName("Animation").id())
        return std::make_shared<AnimNodeData<Animation>>();
    else if (typeId == QMetaType::fromName("PoseSequence").id())
        return std::make_shared <AnimNodeData<PoseSequence>>();
    else
        throw "Unknown Datatype";
}