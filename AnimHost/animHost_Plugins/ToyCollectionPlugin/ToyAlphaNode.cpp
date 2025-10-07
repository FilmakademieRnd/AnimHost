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



#include "ToyAlphaNode.h"
#include <QtWidgets>

ToyAlphaNode::ToyAlphaNode(const QTimer& tick)
{
    /*
    * Initialize all neccessary variables, especially the output data of the node.
    * Leave to widget initialization to embeddedWidget().
    */

    _widget = nullptr;
    _animationOut = std::make_shared<AnimNodeData<Animation>>();
    connect(&tick, &QTimer::timeout, this, &ToyAlphaNode::run);

    qDebug() << "ToyAlphaNode created";
}

ToyAlphaNode::~ToyAlphaNode()
{
    qDebug() << "~ToyAlphaNode()";
}

unsigned int ToyAlphaNode::nDataPorts(QtNodes::PortType portType) const
{
    /*
    * Return the number of data ports for the given port type. In or Out.
    * The number of ports can be dynamic, but must match the dataPortType() function.
    */
    if (portType == QtNodes::PortType::In)
        return 1;
    else
        return 1;
}

NodeDataType ToyAlphaNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    /*
    * Return the data port type for the given port type and port index.
    * This function must match the nDataPorts() function.
    */
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return AnimNodeData<Animation>::staticType();
    else
        return AnimNodeData<Animation>::staticType();
}



void ToyAlphaNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    qDebug() << "ToyAlphaNode run";
}

std::shared_ptr<NodeData> ToyAlphaNode::processOutData(QtNodes::PortIndex port)
{
    /*
    * return processed data based on the rquested output port. Returned data type must match the dataPortType() function.
    */
    return _animationOut;
}

void ToyAlphaNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    /*
    * Check if the incoming data is valid and cast it to the correct type based on the previously defined types in dataPortType() function.
    * If the data is valid, store it in a member variable for further processing.
    * If data is invalid, emit dataInvalidated() to notify the downstream nodes.
    * We recommend handling incoming data as weak_ptr.
    */

    if (!data) {
        Q_EMIT dataInvalidated(0);
    }
    _animationIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
   

    qDebug() << "ToyAlphaNode setInData";
}

bool ToyAlphaNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return !_animationIn.expired();
}

QWidget* ToyAlphaNode::embeddedWidget()
{
    /*
    * Return the embedded widget of the node. This can be a QWidget or nullptr if no widget is embedded.
    * To propagate dynamic changes to the widget size, call Q_EMIT embeddedWidgetSizeUpdated() whenever the widget size changes.
    */

    return nullptr;
}
