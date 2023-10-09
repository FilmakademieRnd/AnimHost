
#include "TracerUpdateSenderPlugin.h"
#include "AnimHostMessageSender.h"
#include "TickReceiver.h"


TracerUpdateSenderPlugin::TracerUpdateSenderPlugin()
{
    _pushButton = nullptr;

    timer = new QTimer();
    timer->setTimerType(Qt::PreciseTimer);
    timer->start(0);
    _updateSenderContext = new zmq::context_t(1);

    //msgSender = new AnimHostMessageSender("127.0.0.1", false, _updateSenderContext); // It could be an idea to let the user set the IP Address using a widget

    //_updateSenderSocket = new zmq::socket_t(*_updateSenderContext, zmq::socket_type::pub);

    //QObject::connect(tickReceiver, &ZMQMessageHandler::stopped, this, &TracerUpdateSenderPlugin::stoppeddddd);

    qDebug() << "TracerUpdateSenderPlugin created";
}

TracerUpdateSenderPlugin::~TracerUpdateSenderPlugin()
{
    if (msgSender)
        msgSender->~AnimHostMessageSender();
    //_updateSenderSocket->close();
    _updateSenderContext->close();
    timer->stop();

    qDebug() << "~TracerUpdateSenderPlugin()";
}

//unsigned int TracerUpdateSenderPlugin::nPorts(QtNodes::PortType portType) const
//{
//    if (portType == QtNodes::PortType::In)
//        // 3 INPUT ports: 1 Animation (pos/rot/scale of bones) + 1 Skeleton (to access bone hierarchy), 1 Pose (position of character joints in character space)
//        return 3;
//    else            
//        // No OUTPUT ports, output stream on ZeroMQ Socket
//        return 0;
//}

unsigned int TracerUpdateSenderPlugin::nDataPorts(QtNodes::PortType portType) const {
    if (portType == QtNodes::PortType::In)
        return 3;
    else
        return 0;
}

//NodeDataType TracerUpdateSenderPlugin::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
//{
//    NodeDataType type;
//    if (portType == QtNodes::PortType::In)  // INPUT Ports DataTypes
//        if (portIndex == 0)
//            return AnimNodeData<Skeleton>::staticType();
//        else if (portIndex == 1)
//            return AnimNodeData<Animation>::staticType();
//        else if (portIndex == 2)
//            return AnimNodeData<Pose>::staticType();
//        else
//            return type;
//    else                                    // OUTPUT Ports DataTypes 
//        return type;
//}

NodeDataType TracerUpdateSenderPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    NodeDataType type;
    if (portType == QtNodes::PortType::In)  // INPUT Ports DataTypes
        if (portIndex == 0)
            return AnimNodeData<Skeleton>::staticType();
        else if (portIndex == 1)
            return AnimNodeData<Animation>::staticType();
        else if (portIndex == 2)
            return AnimNodeData<Pose>::staticType();
        else
            return type;
    else                                    // OUTPUT Ports DataTypes 
        return type;
}

void TracerUpdateSenderPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) {

    if (!data) {
        switch (portIndex) {
            case 0:
                _skeletonIn.reset();
                break;
            case 1:
                _animIn.reset();
                break;
            case 2:
                _poseIn.reset();
                break;

            default:
                return;
        }
        return;
    }

    switch (portIndex) {
        case 0:
            _skeletonIn = std::static_pointer_cast<AnimNodeData<Skeleton>>(data);
            break;
        case 1:
            _animIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
            break;
        case 2:
            _poseIn = std::static_pointer_cast<AnimNodeData<Pose>>(data);
            break;

        default:
            return;
    }

    qDebug() << "TracerUpdateSenderPlugin setInData";
}

void TracerUpdateSenderPlugin::run() {

    qDebug() << "TracerUpdateSenderPlugin running...";

    auto sp_skeleton = _skeletonIn.lock();
    auto sp_animation = _animIn.lock();
    
    // if either skeleton or animation data are NOT set and valid
    if (!sp_skeleton || !sp_animation) {
        return;
    }

    localTime = timer->interval() % 128;

    zeroMQTickReceiverThread = new QThread();
    tickReceiver = new TickReceiver(this, ipAddress, false, _updateSenderContext);

    tickReceiver->moveToThread(zeroMQTickReceiverThread);
    QObject::connect(tickReceiver, &TickReceiver::tick, this, &TracerUpdateSenderPlugin::ticked);
    QObject::connect(zeroMQTickReceiverThread, &QThread::started, tickReceiver, &TickReceiver::run);
    tickReceiver->requestStart();
    zeroMQTickReceiverThread->start();

    msgSender = new AnimHostMessageSender(ipAddress, false, _updateSenderContext);
    zeroMQSenderThread = new QThread();

    //std::vector<float> vec3Example = { 2.5, -7.4, 0 }; // To be substituted with REAL DATA (from sp_akeleton and sp_animation)
    //zmq::message_t* msgVec3 = msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE,
    //                                              0, 0, 0, ZMQMessageHandler::ParameterType::VECTOR3, vec3Example);

    /*float floatExample = 78.3;
    zmq::message_t* msgFloat = msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE,
                                                       0, 0, 0, ZMQMessageHandler::ParameterType::FLOAT, floatExample);*/

    bool boolExample = true;
    zmq::message_t* msgBool = msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE,
                                                      0, 0, 0, ZMQMessageHandler::ParameterType::BOOL, boolExample);

    /*int intExample = -64;
    zmq::message_t* msgInt = msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE,
                                                   0, 0, 0, ZMQMessageHandler::ParameterType::INT, intExample);*/

    msgSender->moveToThread(zeroMQSenderThread);
    QObject::connect(zeroMQSenderThread, &QThread::started, msgSender, &AnimHostMessageSender::run);

    msgSender->requestStart();
    zeroMQSenderThread->start();

    // Example of message creation and 
    msgSender->setMessage(msgBool);
    //msgSender->setMessage(msgInt);
    //msgSender->setMessage(msgFloat);
    //msgSender->setMessage(msgVec3);
}

// When TICK is received message sender is enabled
// This Tick-Slot is connected to the Tick-Signal in the TickRecieverThread
void TracerUpdateSenderPlugin::ticked(int externalTime) {
    localTime = externalTime;
    timer->stop();
    timer->start(localTime);
    msgSender->resume();
}

std::shared_ptr<NodeData> TracerUpdateSenderPlugin::outData(QtNodes::PortIndex port)
{
	return nullptr;
}

std::shared_ptr<NodeData> TracerUpdateSenderPlugin::processOutData(QtNodes::PortIndex port) {
    return nullptr;
}

QWidget* TracerUpdateSenderPlugin::embeddedWidget()
{
    return nullptr;
}

/*void TracerUpdateSenderPlugin::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}*/
