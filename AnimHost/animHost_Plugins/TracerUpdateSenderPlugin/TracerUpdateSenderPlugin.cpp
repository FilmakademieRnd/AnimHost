
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

unsigned int TracerUpdateSenderPlugin::nPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        // 3 INPUT ports: 1 Animation (pos/rot/scale of bones) + 1 Skeleton (to access bone hierarchy), 1 Pose (position of character joints in character space)
        return 3;
    else            
        // No OUTPUT ports, output stream on ZeroMQ Socket
        return 0;
}

NodeDataType TracerUpdateSenderPlugin::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
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

void TracerUpdateSenderPlugin::setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    if (!data) {
        Q_EMIT dataInvalidated(0);
    }

    qDebug() << "Is input data valid? " << validData;
    // InData is READY to be processed if the FIRST TWO ports have been set
    if (validData < 1 && portIndex == 0) {
        validData++;
    } else if (validData < 1 && portIndex == 1) {
        validData++;
    }
    
    if (validData == 1) {
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

        byte posVecExample[3] {2.5, 7.4, 0}; // To be substituted with REAL DATA
        zmq::message_t msg = msgSender->createMessage(ipAddress[ipAddress.size()-1].toLatin1(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE,
                                 0, 0, 0, ZMQMessageHandler::ParameterType::VECTOR3, posVecExample);

        //byte numbers[] = { 10, 5,2,0, 3,7,25, 2,2,2 }; // Example of sending pos/rot/scale data
        //zmq::message_t* msg = new zmq::message_t(static_cast<void*>(numbers), sizeof(numbers));

        // Example of message creation and sending
        msgSender->setMessage(&msg);

        msgSender->moveToThread(zeroMQSenderThread);
        QObject::connect(zeroMQSenderThread, &QThread::started, msgSender, &AnimHostMessageSender::run);

        msgSender->requestStart();
        zeroMQSenderThread->start();
    }
    
    // INPUT DATA PROCESSING
    //_animIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
    //if (auto spAnimation = _animIn.lock()) {
        // Processing animation data, converting it into desired format
        // _animOut = ...
    //}
    //_poseIn = std::static_pointer_cast<AnimNodeData<Pose>>(data);
    //if (auto spPose = _poseIn.lock()) {
        // Processing pose data, converting it into desired format
        // _animOut = ...
    //}

    qDebug() << "TracerUpdateSenderPlugin setInData";
    qDebug() << "Is input data valid? " << validData;


}

void TracerUpdateSenderPlugin::run() {
    // To be populated when the GUI will provide a global "run" signal
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

QWidget* TracerUpdateSenderPlugin::embeddedWidget()
{
    return nullptr;
}

/*void TracerUpdateSenderPlugin::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}*/
