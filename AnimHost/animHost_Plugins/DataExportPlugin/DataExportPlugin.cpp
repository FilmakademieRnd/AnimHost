
#include "DataExportPlugin.h"
#include <QPushButton>

DataExportPlugin::DataExportPlugin()
{
    _pushButton = nullptr;
    qDebug() << "DataExportPlugin created";
}

DataExportPlugin::~DataExportPlugin()
{
    qDebug() << "~DataExportPlugin()";
}

unsigned int DataExportPlugin::nPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType DataExportPlugin::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void DataExportPlugin::setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "DataExportPlugin setInData";
}

std::shared_ptr<NodeData> DataExportPlugin::outData(QtNodes::PortIndex port)
{
	return nullptr;
}

QWidget* DataExportPlugin::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &DataExportPlugin::onButtonClicked);
	}

	return _pushButton;
}

void DataExportPlugin::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}
