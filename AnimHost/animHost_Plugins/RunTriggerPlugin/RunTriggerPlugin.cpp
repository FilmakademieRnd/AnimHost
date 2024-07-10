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

 

#include "RunTriggerPlugin.h"
#include "FrameRange.h"
#include <QPushButton>

RunTriggerPlugin::RunTriggerPlugin()
{
    _pushButton = nullptr;
    qDebug() << "RunTriggerPlugin created";
}

RunTriggerPlugin::~RunTriggerPlugin()
{
    qDebug() << "~RunTriggerPlugin()";
}

unsigned int RunTriggerPlugin::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType RunTriggerPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void RunTriggerPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "RunTriggerPlugin setInData";
}

std::shared_ptr<NodeData> RunTriggerPlugin::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

QWidget* RunTriggerPlugin::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Run");
		_pushButton->resize(QSize(50, 30));

		connect(_pushButton, &QPushButton::released, this, &RunTriggerPlugin::onButtonClicked);
	}

    _pushButton->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
        "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
        "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
        "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
    );

	return _pushButton;
}

void RunTriggerPlugin::onButtonClicked()
{
	qDebug() << "Run Clicked";

    emitRunNextNode();
}
