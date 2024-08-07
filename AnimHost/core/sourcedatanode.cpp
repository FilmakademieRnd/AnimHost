/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *    
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *    
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program; 
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */

 
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


