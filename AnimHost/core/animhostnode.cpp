#include "animhostnode.h"

AnimHostNode::AnimHostNode(std::shared_ptr<PluginInterface> plugin)
{
    _plugin = plugin;
}


unsigned int AnimHostNode::nPorts(PortType portType) const
{
    unsigned int result;

    if (portType == PortType::In)
        result = _plugin->inputs.length();
    else
        result = _plugin->outputs->length();

    return result;
}

NodeDataType AnimHostNode::dataType(PortType portType, PortIndex index) const
{
    QMetaType metaType;
    if (portType == PortType::In)
        metaType = QMetaType(_plugin->inputs[index].userType());
    else
        metaType = QMetaType(_plugin->outputs->at(index).userType());

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

    if (portIndex < _dataIn.size()){
        _dataIn[portIndex] = data;
    }


    compute();
}

void AnimHostNode::compute()
{
//    foreach (var, container) {

//    }
    _plugin->run();
}


NodeDataType AnimHostNode::convertQMetaTypeToNodeDataType(QMetaType qType) const
{
    int typeId = qType.id();
    if (typeId == QMetaType::Float)
        return FloatData().type();
    else if (typeId == QMetaType::fromName("HumanoidBones").id())
        return HumanoidBonesData().type();
    else
        throw "Unknown Datatype";
}

