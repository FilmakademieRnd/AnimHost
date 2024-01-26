
#include "TracerUpdateSenderPlugin.h"
#include "AnimHostMessageSender.h"
#include "TickReceiver.h"


#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

TracerUpdateSenderPlugin::TracerUpdateSenderPlugin()
{
    _sendUpdateButton = nullptr;
    widget = nullptr;
    _selectIPAddress = nullptr;
    _loopCheck = nullptr;
    _ipAddressLayout = nullptr;
    _ipAddress = nullptr;
    _ipValidator = nullptr;

    timer = new QTimer();
    timer->setTimerType(Qt::PreciseTimer);
    timer->start(0);
    _updateSenderContext = new zmq::context_t(1);

    localTime = timer->interval() % 120;

    if (!msgSender) // trying to avoid multiple instances
        msgSender = new AnimHostMessageSender(false, _updateSenderContext);
    if (!zeroMQSenderThread) // trying to avoid multiple instances
        zeroMQSenderThread = new QThread();

    msgSender->moveToThread(zeroMQSenderThread);
    QObject::connect(zeroMQSenderThread, &QThread::started, msgSender, &AnimHostMessageSender::run);

    if(!tickReceiver)
        tickReceiver = new TickReceiver(this, false, _updateSenderContext);
    if(!zeroMQTickReceiverThread)
        zeroMQTickReceiverThread = new QThread();

    tickReceiver->moveToThread(zeroMQTickReceiverThread);
    QObject::connect(tickReceiver, &TickReceiver::tick, this, &TracerUpdateSenderPlugin::ticked);
    QObject::connect(zeroMQTickReceiverThread, &QThread::started, tickReceiver, &TickReceiver::run);

    qDebug() << "TracerUpdateSenderPlugin created";
}

TracerUpdateSenderPlugin::~TracerUpdateSenderPlugin()
{
    zeroMQSenderThread->quit(); zeroMQSenderThread->wait();
    zeroMQTickReceiverThread->quit(); zeroMQTickReceiverThread->wait();

    if (msgSender)
        msgSender->~AnimHostMessageSender();
    _updateSenderContext->close();
    timer->stop();

    qDebug() << "~TracerUpdateSenderPlugin()";
}

unsigned int TracerUpdateSenderPlugin::nDataPorts(QtNodes::PortType portType) const {
    if (portType == QtNodes::PortType::In)
        return 3;
    else
        return 0;
}

NodeDataType TracerUpdateSenderPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    NodeDataType type;
    if (portType == QtNodes::PortType::In)  // INPUT Ports DataTypes
        if (portIndex == 0)
            return AnimNodeData<Animation>::staticType();
        else if (portIndex == 1)
            return AnimNodeData<CharacterObject>::staticType();
        else if (portIndex == 2)
            return AnimNodeData<SceneNodeObjectSequence>::staticType();
        else
            return type;
    else                                    // OUTPUT Ports DataTypes 
        return type;
}

void TracerUpdateSenderPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) {

    if (!data) {
        switch (portIndex) {
            case 0:
                _animIn.reset();
                break;
            case 1:
                _characterIn.reset();
                break;
            case 2:
                _sceneNodeListIn.reset();
                break;

            default:
                return;
        }
        return;
    }

    switch (portIndex) {
        case 0:
            _animIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
            break;
        case 1:
            _characterIn = std::static_pointer_cast<AnimNodeData<CharacterObject>>(data);
            break;
        case 2:
            _sceneNodeListIn = std::static_pointer_cast<AnimNodeData<SceneNodeObjectSequence>>(data);
            break;

        default:
            return;
    }

    qDebug() << "TracerUpdateSenderPlugin setInData";
}

void TracerUpdateSenderPlugin::run() {
    qDebug() << "TracerUpdateSenderPlugin running...";

    // every time the buffer is **predicted to be** half-read, fill the next half buffer
    // TO BE UNCOMMENTED AS SOON AS I RECEIVE bufferSize and renderingFrameRate from receiver
    /*if (localTime % (bufferReadTime / 2) == 0) {
        msgSender->resume();
    }*/
}

// When TICK is received message sender is enabled
// This Tick-Slot is connected to the Tick-Signal in the TickRecieverThread
void TracerUpdateSenderPlugin::ticked(int externalTime) {
    if (std::abs(externalTime - localTime) > 50) {
        localTime = externalTime;
        timer->stop();
        timer->start(localTime);
    }
    if(zeroMQSenderThread->isRunning())
        msgSender->resume();
}

std::shared_ptr<NodeData> TracerUpdateSenderPlugin::outData(QtNodes::PortIndex port)
{
	return nullptr;
}

std::shared_ptr<NodeData> TracerUpdateSenderPlugin::processOutData(QtNodes::PortIndex port) {
    return nullptr;
}

QWidget* TracerUpdateSenderPlugin::embeddedWidget() {
    if (!widget) {
        _sendUpdateButton = new QPushButton("Send Animation");
        _selectIPAddress = new QComboBox();
        _loopCheck = new QCheckBox("Loop");
        _ipAddress = ZMQMessageHandler::getIPList().at(0).toString();
        _ipValidator = new QRegularExpressionValidator(ZMQMessageHandler::ipRegex, this);

        connect(_selectIPAddress, &QComboBox::currentIndexChanged, this, &TracerUpdateSenderPlugin::onChangedSelection);
        connect(_loopCheck, &QCheckBox::stateChanged, this, &TracerUpdateSenderPlugin::onLoopCheck);

        for (QHostAddress ipAddress : ZMQMessageHandler::getIPList()) {
            _selectIPAddress->addItem(ipAddress.toString());
        }
        _selectIPAddress->setValidator(_ipValidator);

        _sendUpdateButton->resize(QSize(30, 30));
        _ipAddressLayout = new QHBoxLayout();

        _ipAddressLayout->addWidget(_selectIPAddress);
        _ipAddressLayout->addWidget(_loopCheck);
        _ipAddressLayout->addWidget(_sendUpdateButton);

        _ipAddressLayout->setSizeConstraint(QLayout::SetMinimumSize);

        widget = new QWidget();
        widget->setLayout(_ipAddressLayout);
        connect(_sendUpdateButton, &QPushButton::released, this, &TracerUpdateSenderPlugin::onButtonClicked);

        widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
                              "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
                              "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
                              "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
        );

    }

    return widget;
}

void TracerUpdateSenderPlugin::onChangedSelection(int index) {
    qDebug() << "IP Address Selection Changed";
    if (index >= 0) {
        ZMQMessageHandler::setIPAddress(ZMQMessageHandler::getIPList().at(index).toString());
        emitDataUpdate(0);
    } else {
        emitDataInvalidated(0);
    }
    qDebug() << "New IP Address:" << ZMQMessageHandler::getOwnIP();
}

void TracerUpdateSenderPlugin::onLoopCheck(int state) {
    // loop is false when checkbox is unchecked, true otherwise
    msgSender->loop = (state != Qt::Unchecked);
}

void TracerUpdateSenderPlugin::onButtonClicked() {
    auto sp_character = _characterIn.lock();
    auto sp_animation = _animIn.lock();
    auto sp_sceneNodeList = _sceneNodeListIn.lock();

    // abort if any shared pointer is invalid
    assert(sp_character && sp_animation && sp_sceneNodeList);

    // Set animation, character and scene data (necessary for creating a pose update message) in the message sender object
    msgSender->setAnimationAndSceneData(sp_animation->getData(), sp_character->getData(), sp_sceneNodeList->getData());

    ///! DEBUGGING SAMPLE DATA USED TO TEST SENDING

    /*bool boolExample = true;
    QByteArray msgBodyBool = msgSender->createMessageBody(0, 0, 0, ZMQMessageHandler::ParameterType::BOOL, boolExample);*/

    /*int intExample = -64;
    QByteArray msgBodyInt = msgSender->createMessageBody(0, 0, 0, ZMQMessageHandler::ParameterType::INT, intExample);*/

    /*float floatExample = 78.3;
    QByteArray msgBodyFloat = msgSender->createMessageBody(0, 0, 0, ZMQMessageHandler::ParameterType::FLOAT, floatExample);*/

    //std::vector<float> vec3Example = { 2.5, -7.4, 0 };
    //QByteArray msgBodyVec3 = msgSender->createMessageBody(0, 0, 0, ZMQMessageHandler::ParameterType::VECTOR3, vec3Example);

   /* std::vector<float> quatExample = { -0.38, -0.07, 0.16, 0.91 };
    std::vector<float> quatExample2 = { 0.06, 0.5, 0.86, 0.11 };
    QByteArray msgBodyQuat = msgSender->createMessageBody(254, 3, 3, ZMQMessageHandler::ParameterType::QUATERNION, quatExample);
    QByteArray msgQuat2 = msgSender->createMessageBody(254, 3, 47, ZMQMessageHandler::ParameterType::QUATERNION, quatExample2);
    msgBodyQuat.append(msgQuat2);*/

    if (!zeroMQSenderThread->isRunning()) {
        msgSender->requestStart();
        zeroMQSenderThread->start();
    } else {
        msgSender->resume();
    }   

    // Example of message creation
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyBool));
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyInt));
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyFloat));
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyVec3));
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyQuat));
}
