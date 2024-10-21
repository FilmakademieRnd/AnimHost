#include "RPCTriggerNode.h"
#include <QPushButton>

RPCTriggerNode::RPCTriggerNode()
{
    _pushButton = nullptr;
    //qDebug() << "RPCTriggerNode created";
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
    
    run();

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
    return true;
}

void RPCTriggerNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    //qDebug() << "RPCTriggerNode run";
    if(isDataAvailable()){
        
        if(auto RPCData = _RPCIn.lock()){
			//qDebug() << "RPCData Received";

            auto sp_rpc = RPCData->getData();

            if (sp_rpc->sceneID == 255 && sp_rpc->objectID == 1 && sp_rpc->paramID == 0 && sp_rpc->paramType == ZMQMessageHandler::INT) {
                QByteArray data = sp_rpc->rawData;

                uint32_t rpc;
                //convert data to int
                std::memcpy(&rpc, data.data(), sizeof(uint32_t));

                if(rpc == _filterType){
					qDebug() << "RPCData Received: " << rpc;
                    emitRunNextNode();
				}

            }
		}
		else{
			qDebug() << "RPCData not Received";
		}
    }
}



QWidget* RPCTriggerNode::embeddedWidget()
{
	if (!_widget) {

        _widget = new QWidget();
        _mainLayout = new QVBoxLayout(_widget);

        _comboBox = new QComboBox();

        _comboBox->addItem("STOP", QVariant::fromValue(AnimHostRPCType::STOP));
        _comboBox->addItem("STREAM", QVariant::fromValue(AnimHostRPCType::STREAM));
        _comboBox->addItem("STREAM_LOOP", QVariant::fromValue(AnimHostRPCType::STREAM_LOOP));
        _comboBox->addItem("BLOCK", QVariant::fromValue(AnimHostRPCType::BLOCK));
        _comboBox->setCurrentIndex(3);

        _mainLayout->addWidget(_comboBox);

        connect(_comboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
            _filterType = _comboBox->currentData().value<AnimHostRPCType>();
            });

        _pushButton = new QPushButton("Run");
        _mainLayout->addWidget(_pushButton);

		connect(_pushButton, &QPushButton::released, this, &RPCTriggerNode::onButtonClicked);

        _widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
            "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
            "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
            "QLabel{padding: 5px;}"
            "QComboBox{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
            "QComboBox::drop-down{"
            "background-color:rgb(98, 139, 202);"
            "subcontrol-origin: padding;"
            "subcontrol-position: top right;"
            "width: 15px;"
            "border-top-right-radius: 4px;"
            "border-bottom-right-radius: 4px;}"
            "QComboBox QAbstractItemView{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-bottom-right-radius: 4px; border-bottom-left-radius: 4px; padding: 0px;}"
            "QScrollBar:vertical {"
            "border: 1px rgb(25, 25, 25);"
            "background:rgb(25, 25, 25);"
            "border-radius: 2px;"
            "width:6px;"
            "margin: 2px 0px 2px 1px;}"
            "QScrollBar::handle:vertical {"
            "border-radius: 2px;"
            "min-height: 0px;"
            "background-color: rgb(25, 25, 25);}"
            "QScrollBar::add-line:vertical {"
            "height: 0px;"
            "subcontrol-position: bottom;"
            "subcontrol-origin: margin;}"
            "QScrollBar::sub-line:vertical {"
            "height: 0px;"
            "subcontrol-position: top;"
            "subcontrol-origin: margin;}"
        );
	}

	return _widget;
}

void RPCTriggerNode::onButtonClicked()
{
    emitRunNextNode();
}