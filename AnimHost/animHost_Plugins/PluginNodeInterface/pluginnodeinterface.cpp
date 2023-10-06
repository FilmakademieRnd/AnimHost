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

void PluginNodeInterface::setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{	
	if (portIndex == 0 && hasInputRunSignal()) {
		if (data) {
			auto runSignal = std::static_pointer_cast<AnimNodeData<RunSignal>>(data);
			if (runSignal)
				run();
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

void PluginNodeInterface::emitRunNextNode()
{
	if (hasOutputRunSignal()) {
		if (_runSignal) {
			Q_EMIT dataUpdated(0);
		} 
		else {
			_runSignal = std::make_shared<AnimNodeData<RunSignal>>();
			Q_EMIT dataUpdated(0);
		}
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


