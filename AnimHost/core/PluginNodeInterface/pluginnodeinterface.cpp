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

 
#include "pluginnodeinterface.h"
#include <nodedatatypes.h>


unsigned int PluginNodeInterface::nPorts(QtNodes::PortType portType) const
{
	if (portType == QtNodes::PortType::In && hasInputRunSignal()) {
		return 1 + nDataPorts(portType);
	}
	else if (portType == QtNodes::PortType::Out && hasOutputRunSignal()) {
		return 1 + nDataPorts(portType);
	}
	
	return nDataPorts(portType);
}

NodeDataType PluginNodeInterface::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
	if (portType == QtNodes::PortType::In && hasInputRunSignal()) {
		if(portIndex == 0)
			return AnimNodeData<RunSignal>::staticType();

		return dataPortType(portType, portIndex - 1);
	}
	else if (portType == QtNodes::PortType::Out && hasOutputRunSignal()) {
		if (portIndex == 0)
			return AnimNodeData<RunSignal>::staticType();

		return dataPortType(portType, portIndex - 1);
	}

	return dataPortType(portType, portIndex);
}

std::shared_ptr<NodeData> PluginNodeInterface::outData(QtNodes::PortIndex port)
{
	if (hasOutputRunSignal()) {
		if (port == 0) {
			return _runSignal;
		}

		return processOutData(port - 1);
	}
	else {
		return processOutData(port);
	}
}

void PluginNodeInterface::propagateMetadata(const QVariantMap& metadata)
{
	for (auto it = metadata.begin(); it != metadata.end(); ++it) {
		const QByteArray key = it.key().toUtf8();

		if (metaObject()->indexOfProperty(key) != -1) {
			//qDebug() << "Setting property: " << key << " to value: " << it.value();
			setProperty(key, it.value());
		} else {
			//qDebug() << "No property found for key: " << it.key();
		}
	}

}

void PluginNodeInterface::setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{	
	if (portIndex == 0 && hasInputRunSignal()) {
		if (data) {
			_runSignalIncoming = std::static_pointer_cast<AnimNodeData<RunSignal>>(data);
			if (_runSignalIncoming) {
				QVariantMap metadata = _runSignalIncoming->getData()->metadata;
				propagateMetadata(metadata);
				run();
			}
				
		}
		return;
	}
	
	if (hasInputRunSignal()) {
		processInData(data, portIndex - 1);
		return;
	}

	processInData(data, portIndex);
}

void PluginNodeInterface::emitDataUpdate(QtNodes::PortIndex portIndex)
{
	if (hasOutputRunSignal()) {
		Q_EMIT dataUpdated(portIndex + 1);
		return;
	}

	Q_EMIT dataUpdated(portIndex);
}

void PluginNodeInterface::emitRunNextNode(QVariantMap* parameter)
{
	if (hasOutputRunSignal()) {
		if (!_runSignal) {
			_runSignal = std::make_shared<AnimNodeData<RunSignal>>();
		} 

		//If available, copy parameter
		if (parameter) {
			_runSignal->getData()->metadata = *parameter;
		}
		else if (_runSignalIncoming) {
			_runSignal->getData()->metadata = _runSignalIncoming->getData()->metadata;
		}

		Q_EMIT dataUpdated(0);
	}
	else {
		qDebug() << "Node has no output run signal.";
	}
	return;
}

void PluginNodeInterface::emitDataInvalidated(QtNodes::PortIndex portIndex)
{
	if (hasOutputRunSignal()) {
		Q_EMIT dataInvalidated(portIndex + 1);
		return;
	}
	
	Q_EMIT dataInvalidated(portIndex);
}


