#include "HelloWorldNode.h"
#include <QPushButton>

HelloWorldNode::HelloWorldNode()
{
    _pushButton = nullptr;
    qDebug() << "HelloWorldNode created";
}

HelloWorldNode::~HelloWorldNode()
{
    qDebug() << "~HelloWorldNode()";
}

unsigned int HelloWorldNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType HelloWorldNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void HelloWorldNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "HelloWorldNode setInData";
}

std::shared_ptr<NodeData> HelloWorldNode::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

bool HelloWorldNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return false;
}

void HelloWorldNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    qDebug() << "HelloWorldNode run";
    if(isDataAvailable()){
        /*
        * Do Stuff
        */
    }
}



QWidget* HelloWorldNode::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &HelloWorldNode::onButtonClicked);
	}

	return _pushButton;
}

void HelloWorldNode::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}