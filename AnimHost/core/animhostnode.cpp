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

    if (portType == PortType::In) {
        result = _plugin->inputs.size();
        if (hasInputRunSignal())
            result += 1;
    }
    else {
        result = _plugin->outputs.size();
        if (hasOutputRunSignal())
            result += 1;
    }
    return result;
}


NodeDataType AnimHostNode::dataType(PortType portType, PortIndex index) const
{
    QMetaType metaType;

    if (portType == PortType::In) {
        if (hasInputRunSignal()) {
            if (index == 0)
                return AnimNodeData<RunSignal>::staticType();

            metaType = QMetaType(_plugin->inputs[index - 1]);
        }
        else {
            metaType = QMetaType(_plugin->inputs[index]);
        }


    }
    else if (portType == PortType::Out){
        if (hasOutputRunSignal()) {
            if(index == 0)
                return AnimNodeData<RunSignal>::staticType();

            metaType = QMetaType(_plugin->outputs[index - 1]);
        }
        else {
            metaType = QMetaType(_plugin->outputs[index]);
        }
    }


    return convertQMetaTypeToNodeDataType(metaType);
}

std::shared_ptr<NodeData> AnimHostNode::outData(PortIndex port)
{
    if (hasOutputRunSignal()) {
        if (port < _dataOut.size() + 1) {
            if (port == 0) {
                return _runSignal;
            }

            return std::static_pointer_cast<NodeData>(_dataOut[port - 1]);
        }
        else {
            throw "PortIndex out of range!";
        }
    }
    else {
        if (port < _dataOut.size()) {
            return std::static_pointer_cast<NodeData>(_dataOut[port]);
        }
        else {
            throw "PortIndex out of range!";
        }
    }

}


void AnimHostNode::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    if (hasInputRunSignal()) {
        if (portIndex == 0) {
            if (data) {
                auto runIn = std::static_pointer_cast<AnimNodeData<RunSignal>>(data);
                _runSignal = runIn;
                compute();
                return;
            }
            else {
                _runSignal = nullptr;
                return;
            }
        }    
        processInData(data, portIndex - 1);
    }
    else {
        processInData(data, portIndex);
    }
}

void AnimHostNode::emitDataUpdate(QtNodes::PortIndex portIndex)
{
    if (hasOutputRunSignal()) {
        Q_EMIT dataUpdated(portIndex + 1);
        return;
    }

    Q_EMIT dataUpdated(portIndex);
}

void AnimHostNode::emitRunNextNode()
{
    if (hasOutputRunSignal()) {
        Q_EMIT dataUpdated(0);
    }
    else {
        qDebug() << "Node has no output run signal.";
    }
    return;
}

void AnimHostNode::emitDataInvalidated(QtNodes::PortIndex portIndex)
{
    if (hasOutputRunSignal()) {
        Q_EMIT dataInvalidated(portIndex + 1);
        return;
    }

    Q_EMIT dataInvalidated(portIndex);
}

NodeDataType AnimHostNode::convertQMetaTypeToNodeDataType(QMetaType qType)
{

    int typeId = qType.id();

    if (typeId == QMetaType::fromName("Pose").id())
        return AnimNodeData<Pose>::staticType();
    else if (typeId == QMetaType::fromName("Skeleton").id())
        return AnimNodeData<Skeleton>::staticType();
    else if (typeId == QMetaType::fromName("Animation").id())
        return AnimNodeData<Animation>::staticType();
    else if (typeId == QMetaType::fromName("PoseSequence").id())
        return AnimNodeData<PoseSequence>::staticType();
    else if (typeId == QMetaType::fromName("JointVelocitySequence").id())
        return AnimNodeData<JointVelocitySequence>::staticType();
    else
        throw "Unknown Datatype";
}


std::shared_ptr<AnimNodeDataBase> AnimHostNode::createAnimNodeDataFromID(QMetaType qType)
{
    int typeId = qType.id();

    if (typeId == QMetaType::fromName("Pose").id())
        return std::make_shared<AnimNodeData<Pose>>();
    else if (typeId == QMetaType::fromName("shared_ptr<Pose>").id())
        return std::make_shared<AnimNodeData<Pose>>();
    else if (typeId == QMetaType::fromName("Skeleton").id())
        return std::make_shared<AnimNodeData<Skeleton>>();
    else if (typeId == QMetaType::fromName("Animation").id())
        return std::make_shared<AnimNodeData<Animation>>();
    else if (typeId == QMetaType::fromName("PoseSequence").id())
        return std::make_shared <AnimNodeData<PoseSequence>>();
    else if (typeId == QMetaType::fromName("JointVelocitySequence").id())
        return std::make_shared <AnimNodeData<JointVelocitySequence>>();
    else
        throw "Unknown Datatype";
}