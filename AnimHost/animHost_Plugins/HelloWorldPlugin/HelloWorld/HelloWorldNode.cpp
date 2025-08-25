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

#include "HelloWorldNode.h"
#include <QPushButton>

HelloWorldNode::HelloWorldNode()
{
    _pushButton = nullptr;
    _widget = nullptr;
    
    qDebug() << "HelloWorldNode created";
}

HelloWorldNode::~HelloWorldNode()
{
    qDebug() << "~HelloWorldNode()";
}

unsigned int HelloWorldNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;  // No custom input ports
    else
        return 0;  // No output ports
}

NodeDataType HelloWorldNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    return NodeDataType{};
}

void HelloWorldNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    // No custom input ports - framework handles RunSignal automatically
    // This method won't be called since nDataPorts returns 0 for input
    qDebug() << "HelloWorldNode processInData called unexpectedly";
}

std::shared_ptr<NodeData> HelloWorldNode::processOutData(QtNodes::PortIndex port)
{
    return nullptr;  // No output data
}

bool HelloWorldNode::isDataAvailable() 
{
    // We can always run - either from button click or run signal
    return true;
}

void HelloWorldNode::run()
{
    qDebug() << "HelloWorldNode run - Hello World!";
}

QWidget* HelloWorldNode::embeddedWidget()
{
    if (!_pushButton) {
        _pushButton = new QPushButton("Say Hello");
        _pushButton->resize(QSize(120, 40));
        
        connect(_pushButton, &QPushButton::released, this, &HelloWorldNode::onButtonClicked);
    }
    
    return _pushButton;
}

void HelloWorldNode::onButtonClicked()
{
    qDebug() << "Hello World button clicked!";
    
    // Run the node when button is clicked
    run();
}