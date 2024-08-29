#include "SceneRecieverNode.h"
#include <QPushButton>

SceneRecieverNode::SceneRecieverNode()
{
    _pushButton = nullptr;
    qDebug() << "SceneRecieverNode created";
}

SceneRecieverNode::~SceneRecieverNode()
{
    qDebug() << "~SceneRecieverNode()";
}

unsigned int SceneRecieverNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType SceneRecieverNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void SceneRecieverNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "SceneRecieverNode setInData";
}

std::shared_ptr<NodeData> SceneRecieverNode::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

bool SceneRecieverNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return false;
}

void SceneRecieverNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    qDebug() << "SceneRecieverNode run";
    if(isDataAvailable()){
        /*
        * Do Stuff
        */
    }
}



QWidget* SceneRecieverNode::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &SceneRecieverNode::onButtonClicked);
	}

	return _pushButton;
}

void SceneRecieverNode::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}