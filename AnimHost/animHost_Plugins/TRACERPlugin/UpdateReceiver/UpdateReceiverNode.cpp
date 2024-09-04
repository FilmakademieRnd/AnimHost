#include "UpdateReceiverNode.h"
#include <QPushButton>

UpdateReceiverNode::UpdateReceiverNode(std::shared_ptr<TRACERUpdateReceiver> updateReceiver) : _updateReceiver(updateReceiver)
{
    _connectButton = nullptr;

    qDebug() << "UpdateReceiverNode created";
}

UpdateReceiverNode::~UpdateReceiverNode()
{
    qDebug() << "~UpdateReceiverNode()";
}

unsigned int UpdateReceiverNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType UpdateReceiverNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void UpdateReceiverNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "UpdateReceiverNode setInData";
}

std::shared_ptr<NodeData> UpdateReceiverNode::processOutData(QtNodes::PortIndex port)
{
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

        _mainLayout->addLayout(_ipAddressLayout);

        _autoStart = new QCheckBox("Auto Start");
        _autoStart->setToolTip("Automatically start the TRACER Update Receiver on loading a Node Setup");
        _mainLayout->addWidget(_autoStart);

        _connectButton = new QPushButton("Connect");
        _connectButton->setToolTip("Connect to the TRACER Server");
        _mainLayout->addWidget(_connectButton);


        _widget->setLayout(_mainLayout);

		connect(_connectButton, &QPushButton::released, this, &UpdateReceiverNode::onButtonClicked);

        _widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
            "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
            "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
            "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
        );
	}

	return _widget;
}

void UpdateReceiverNode::onButtonClicked()
{

    QMetaObject::invokeMethod(_updateReceiver.get(), "initializeUpdateReceiverSocket", Qt::QueuedConnection, Q_ARG(QString, _ipAddress->text()));


	qDebug() << "Example Widget Clicked";
}