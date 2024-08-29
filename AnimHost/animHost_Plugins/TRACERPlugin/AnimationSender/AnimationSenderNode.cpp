#include "AnimationSenderNode.h"
#include <QPushButton>

AnimationSenderNode::AnimationSenderNode()
{
    _pushButton = nullptr;
    qDebug() << "AnimationSenderNode created";
}

AnimationSenderNode::~AnimationSenderNode()
{
    qDebug() << "~AnimationSenderNode()";
}

unsigned int AnimationSenderNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType AnimationSenderNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void AnimationSenderNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "AnimationSenderNode setInData";
}

std::shared_ptr<NodeData> AnimationSenderNode::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

bool AnimationSenderNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return false;
}

void AnimationSenderNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    qDebug() << "AnimationSenderNode run";
    if(isDataAvailable()){
        /*
        * Do Stuff
        */
    }
}



QWidget* AnimationSenderNode::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &AnimationSenderNode::onButtonClicked);
	}

	return _pushButton;
}

void AnimationSenderNode::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}