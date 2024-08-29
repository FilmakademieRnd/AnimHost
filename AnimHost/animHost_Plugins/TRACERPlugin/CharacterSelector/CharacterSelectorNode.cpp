#include "CharacterSelectorNode.h"
#include <QPushButton>

CharacterSelectorNode::CharacterSelectorNode()
{
    _pushButton = nullptr;
    qDebug() << "CharacterSelectorNode created";
}

CharacterSelectorNode::~CharacterSelectorNode()
{
    qDebug() << "~CharacterSelectorNode()";
}

unsigned int CharacterSelectorNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType CharacterSelectorNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void CharacterSelectorNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "CharacterSelectorNode setInData";
}

std::shared_ptr<NodeData> CharacterSelectorNode::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

bool CharacterSelectorNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return false;
}

void CharacterSelectorNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    qDebug() << "CharacterSelectorNode run";
    if(isDataAvailable()){
        /*
        * Do Stuff
        */
    }
}



QWidget* CharacterSelectorNode::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &CharacterSelectorNode::onButtonClicked);
	}

	return _pushButton;
}

void CharacterSelectorNode::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}