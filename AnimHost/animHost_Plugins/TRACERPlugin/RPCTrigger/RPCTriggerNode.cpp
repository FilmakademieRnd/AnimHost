#include "RPCTriggerNode.h"
#include <QPushButton>

RPCTriggerNode::RPCTriggerNode()
{
    _pushButton = nullptr;
    qDebug() << "RPCTriggerNode created";
}

RPCTriggerNode::~RPCTriggerNode()
{
    qDebug() << "~RPCTriggerNode()";
}

unsigned int RPCTriggerNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 1;
    else            
        return 0;
}

NodeDataType RPCTriggerNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return AnimNodeData<RPCUpdate>::staticType();
    else
        return type;
}

void RPCTriggerNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{

    if (!data) {
        Q_EMIT dataInvalidated(0);
    }

    _RPCIn = std::static_pointer_cast<AnimNodeData<RPCUpdate>>(data);

    qDebug() << "RPCTriggerNode setInData";
}

std::shared_ptr<NodeData> RPCTriggerNode::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

bool RPCTriggerNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return false;
}

void RPCTriggerNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    qDebug() << "RPCTriggerNode run";
    if(isDataAvailable()){
        
        if(auto RPCData = _RPCIn.lock()){
			qDebug() << "RPCData Received";

            emitRunNextNode();
		}
		else{
			qDebug() << "RPCData not Received";
		}
    }
}



QWidget* RPCTriggerNode::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &RPCTriggerNode::onButtonClicked);
	}

	return _pushButton;
}

void RPCTriggerNode::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}