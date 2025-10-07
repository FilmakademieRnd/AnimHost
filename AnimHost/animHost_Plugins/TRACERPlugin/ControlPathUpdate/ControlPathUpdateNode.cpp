#include "ControlPathUpdateNode.h"
#include <QPushButton>

ControlPathUpdateNode::ControlPathUpdateNode()
{
    _pushButton = nullptr;
    qDebug() << "ControlPathUpdateNode created";
}

ControlPathUpdateNode::~ControlPathUpdateNode()
{
    qDebug() << "~ControlPathUpdateNode()";
}

unsigned int ControlPathUpdateNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType ControlPathUpdateNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void ControlPathUpdateNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "ControlPathUpdateNode setInData";
}

std::shared_ptr<NodeData> ControlPathUpdateNode::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

bool ControlPathUpdateNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return false;
}

void ControlPathUpdateNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    qDebug() << "ControlPathUpdateNode run";
    if(isDataAvailable()){
        /*
        * Do Stuff
        */
    }
}



QWidget* ControlPathUpdateNode::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &ControlPathUpdateNode::onButtonClicked);
	}

	return _pushButton;
}

void ControlPathUpdateNode::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}