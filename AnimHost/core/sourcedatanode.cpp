#include "sourcedatanode.h"

SourceDataNode::SourceDataNode()
{

    _dataOut.push_back(std::make_shared<AnimNodeData<Pose>>());



}


unsigned int SourceDataNode::nPorts(PortType portType) const
{
    unsigned int result;

    if (portType == PortType::In)
        result = 0;
    else
        result = _dataOut.size();

    return result;
}

NodeDataType SourceDataNode::dataType(PortType portType, PortIndex index) const
{
    NodeDataType type;
    if (portType == PortType::In)
        return type;
    else
        return type = _dataOut[index]->type();

}

std::shared_ptr<NodeData> SourceDataNode::outData(PortIndex index)
{
    //QVariant data = _plugin->outputs->at(index);

    if(index < _dataOut.size()){
        return std::static_pointer_cast<NodeData>(_dataOut[index]);
    }

    else{
        throw "PortIndex out of range!";
    }

}


