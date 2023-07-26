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

std::shared_ptr<NodeData> AnimHostNode::outData(PortIndex index)
{
    //QVariant data = _plugin->outputs->at(index);

    if(index < _dataOut.size()){
        return std::static_pointer_cast<NodeData>(_dataOut[index]);
    }

    else{
        throw "PortIndex out of range!";
    }

}

void AnimHostNode::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    if (!data) {
        Q_EMIT dataInvalidated(0);
    }

    _dataIn[portIndex] = data;

    compute();
}

void AnimHostNode::compute()
{
   
    QVariantList list;
    QVariantList listOut;

    foreach (std::weak_ptr<QtNodes::NodeData> var,  _dataIn)
    {
        auto test = std::dynamic_pointer_cast<AnimNodeData>(var.lock());
        if (!test)
            return;
        auto variant = test->getVariant();

        list.append(variant);
    }
    qDebug() << "AnimHostNode::compute()";
    _plugin->run(list, listOut);

    int counter = 0;
    foreach(QVariant var, listOut)
    {
        auto nodeData = createAnimNodeDataFromID(var.metaType());
        nodeData->setVariant(var);
        _dataOut[counter] = nodeData;
    }

    for (int i = 1; i <= nPorts(PortType::Out); i++)
    {
        Q_EMIT dataUpdated(i - 1);
    }
}


NodeDataType AnimHostNode::convertQMetaTypeToNodeDataType(QMetaType qType) const
{
    int typeId = qType.id();

    if (typeId == QMetaType::Float)
        return FloatData::staticType();
    else if (typeId == QMetaType::Int)
        return IntData::staticType();
    else if (typeId == QMetaType::fromName("HumanoidBones").id())
        return HumanoidBonesData::staticType();
    else if (typeId == QMetaType::fromName("Pose").id())
        return PoseNodeData::staticType();
    //else if (typeId == QMetaType::fromName("shared_ptr<Pose>").id())
      //  return PoseNodeData::staticType();
    else if (typeId == QMetaType::fromName("Skeleton").id())
        return SkeletonNodeData::staticType();
    else if (typeId == QMetaType::fromName("Animation").id())
        return AnimationNodeData::staticType();
    else
        throw "Unknown Datatype";
}

std::shared_ptr<AnimNodeData> AnimHostNode::createAnimNodeDataFromID(QMetaType qType) const
{
    int typeId = qType.id();

    if (typeId == QMetaType::Float)
        return std::make_shared<FloatData>();
    else if (typeId == QMetaType::Int)
        return std::make_shared<IntData>();
    else if (typeId == QMetaType::fromName("HumanoidBones").id())
        return std::make_shared<HumanoidBonesData>();
    else if (typeId == QMetaType::fromName("Pose").id())
        return std::make_shared<PoseNodeData>();
    else if (typeId == QMetaType::fromName("shared_ptr<Pose>").id())
        return std::make_shared<PoseNodeData>();
    else if (typeId == QMetaType::fromName("Skeleton").id())
        return std::make_shared<SkeletonNodeData>();
    else if (typeId == QMetaType::fromName("Animation").id())
        return std::make_shared<AnimationNodeData>();
    else
        throw "Unknown Datatype";
}