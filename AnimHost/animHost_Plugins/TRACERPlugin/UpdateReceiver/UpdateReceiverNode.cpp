#include "UpdateReceiverNode.h"
#include <QPushButton>

UpdateReceiverNode::UpdateReceiverNode(std::shared_ptr<TRACERUpdateReceiver> updateReceiver) : _updateReceiver(updateReceiver)
{
    _connectButton = nullptr;

    _rpcUpdateOut = std::make_shared<AnimNodeData<RPCUpdate>>();
    _parameterUpdateOut = std::make_shared<AnimNodeData<ParameterUpdate>>();

    connect(_updateReceiver.get(), &TRACERUpdateReceiver::parameterUpdateMessage, 
            this, &UpdateReceiverNode::forwardParameterUpdateMessage, 
            Qt::QueuedConnection);

    connect(_updateReceiver.get(), &TRACERUpdateReceiver::rpcMessage,
        this, &UpdateReceiverNode::forwardRPCMessage,
		Qt::QueuedConnection);

    //qDebug() << "UpdateReceiverNode created";
}

UpdateReceiverNode::~UpdateReceiverNode()
{
    _updateReceiver.reset();
    qDebug() << "~UpdateReceiverNode()";
}

QJsonObject UpdateReceiverNode::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    if(_ipAddress){
		modelJson["ipAddress"] = _ipAddress->text();
	}

    if (_autoStart) {
        modelJson["autoStart"] = _autoStart->isChecked();
    }

    
    return modelJson;
}

void UpdateReceiverNode::load(QJsonObject const& p)
{
    if (p.contains("ipAddress")) {
		_ipAddress->setText(p["ipAddress"].toString());
	}

	if (p.contains("autoStart")) {
		_autoStart->setChecked(p["autoStart"].toBool());
	}

    if(_autoStart->isChecked()){
		_updateReceiver->requestRestart(_ipAddress->text());
	}
}

unsigned int UpdateReceiverNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 2;
}

NodeDataType UpdateReceiverNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In) {
        return type;
    }
    else {
        if (portIndex == 0)
            return AnimNodeData<ParameterUpdate>::staticType();
        else if (portIndex == 1)
            return AnimNodeData<RPCUpdate>::staticType();
    }
        
    return type;
}

void UpdateReceiverNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
   // qDebug() << "UpdateReceiverNode setInData";
}

std::shared_ptr<NodeData> UpdateReceiverNode::processOutData(QtNodes::PortIndex port)
{
    if (port == 0) {
		return _parameterUpdateOut;
	}
	else if (port == 1){
		return _rpcUpdateOut;
	}
	return nullptr;

}

bool UpdateReceiverNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return false;
}

void UpdateReceiverNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    qDebug() << "UpdateReceiverNode run";
    if(isDataAvailable()){
        /*
        * Do Stuff
        */
    }

    emitDataUpdate(0);
}


void UpdateReceiverNode::forwardParameterUpdateMessage(uint8_t sceneID, uint16_t objectID, uint16_t paramID,
    ZMQMessageHandler::ParameterType paramType, const QByteArray rawData)
{
    qDebug() << "PARAM::SceneID: " << sceneID << " ObjectID: " << objectID << " ParamID: " << paramID << " ParamType: " << paramType;

    std::shared_ptr<ParameterUpdate> paramUpdate = std::make_shared<ParameterUpdate>(sceneID, objectID,paramID,
        paramType, rawData);

    _parameterUpdateOut->setVariant(QVariant::fromValue(paramUpdate));

    emitDataUpdate(0);
}

void UpdateReceiverNode::forwardRPCMessage(uint8_t sceneID, uint16_t objectID, uint16_t paramID, ZMQMessageHandler::ParameterType paramType, const QByteArray rawData)
{
    qDebug() << "RPC::SceneID: " << sceneID << " ObjectID: " << objectID << " ParamID: " << paramID << " ParamType: " << paramType;

    std::shared_ptr<RPCUpdate> rpcUpdate = std::make_shared<RPCUpdate>(sceneID, objectID, paramID,
        paramType, rawData);

    _rpcUpdateOut->setVariant(QVariant::fromValue(rpcUpdate));


    emitDataUpdate(1);
}

QWidget* UpdateReceiverNode::embeddedWidget()
{
	if (!_widget) {
        _widget = new QWidget();

        _mainLayout = new QVBoxLayout(_widget);


        _ipAddressLayout = new QHBoxLayout();
        _ipAddress = new QLineEdit();
        QRegularExpression ipRegex(
            R"((^((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])\.){3}(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])$))");

        _ipValidator = new QRegularExpressionValidator(ipRegex, _ipAddress);
        _ipAddress->setValidator(_ipValidator);
        _ipAddress->setPlaceholderText("Enter IP Address");
        _ipAddress->setToolTip("Enter the IP Address of the TRACER Server");
        _ipAddressLayout->addWidget(_ipAddress);    

        _signalLight = new SignalLightWidget();
        _ipAddressLayout->addWidget(_signalLight);

        _mainLayout->addLayout(_ipAddressLayout);

        _autoStart = new QCheckBox("Auto Start");
        _autoStart->setToolTip("Automatically start the TRACER Update Receiver on loading a Node Setup");
        _mainLayout->addWidget(_autoStart);

        _connectButton = new QPushButton("Connect");
        _connectButton->setToolTip("Connect to the TRACER Server");
        _mainLayout->addWidget(_connectButton);


        

        _widget->setLayout(_mainLayout);

		connect(_connectButton, &QPushButton::released, this, &UpdateReceiverNode::onButtonClicked);


        connect(_updateReceiver.get(), &TRACERUpdateReceiver::receiverStatus,
            this, [=](int status) {
                switch(status){
					case 0: // Running
						_signalLight->setColor(QColor(50, 255, 50));
						//_signalLight->startFadeOut(500);
						break;
					case 1: //Parameter Update
						_signalLight->setColor(QColor(255, 200, 50));
						_signalLight->startFadeOut(400, QColor(50, 255, 50));
						break;
					case 2: // RPC
						_signalLight->setColor(QColor(255, 50, 150));
						_signalLight->startFadeOut(400, QColor(50, 255, 50));
						break;
					default:
						_signalLight->setColor(QColor(255, 0, 0));
						//_signalLight->startFadeOut(500);
						break;
				}

            });

        _widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px none white;""}"
            "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
            "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
            "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
        );   
            
	}

	return _widget;
}

void UpdateReceiverNode::onButtonClicked()
{

    _updateReceiver->requestRestart(_ipAddress->text());


	qDebug() << "Example Widget Clicked";
}