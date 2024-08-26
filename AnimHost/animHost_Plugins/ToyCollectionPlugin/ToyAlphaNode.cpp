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
    if (portType == QtNodes::PortType::In)
        return 1;
    else
        return 1;
}

NodeDataType ToyAlphaNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return AnimNodeData<Animation>::staticType();
    else
        return AnimNodeData<Animation>::staticType();
}



void ToyAlphaNode::run()
{
    qDebug() << "ToyAlphaNode run";
}

std::shared_ptr<NodeData> ToyAlphaNode::processOutData(QtNodes::PortIndex port)
{
    return _animationOut;
}

void ToyAlphaNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    if (!data) {
        Q_EMIT dataInvalidated(0);
    }
    _animationIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
   

    qDebug() << "ToyAlphaNode setInData";
}

bool ToyAlphaNode::isDataAvailable() {
    return !_animationIn.expired();
}

QWidget* ToyAlphaNode::embeddedWidget()
{
    /*if (!_widget) {

        _widget = new QWidget();
    }

    _widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
        "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
        "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
        "QLabel{padding: 5px;}"
    );

    return _widget;*/

    return nullptr;
}
