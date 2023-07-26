#include "sourcedatanode.h"

SourceDataNode::SourceDataNode()
{


    auto h = std::make_shared<AnimNodeData<HumanoidBones>>();
    
    h->getData()->SetSpine({ 1.f,2.f,3.f,4.f });

    _dataOut.push_back(h);
 
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


