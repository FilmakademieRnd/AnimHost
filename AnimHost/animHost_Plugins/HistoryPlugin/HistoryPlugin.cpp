
#include "HistoryPlugin.h"
#include <QPushButton>

HistoryPlugin::HistoryPlugin()
{
    _pushButton = nullptr;
    qDebug() << "HistoryPlugin created";
}

HistoryPlugin::~HistoryPlugin()
{
    qDebug() << "~HistoryPlugin()";
}

unsigned int HistoryPlugin::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType HistoryPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void HistoryPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "HistoryPlugin setInData";
}

std::shared_ptr<NodeData> HistoryPlugin::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

QWidget* HistoryPlugin::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &HistoryPlugin::onButtonClicked);
	}

	return _pushButton;
}

void HistoryPlugin::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}
