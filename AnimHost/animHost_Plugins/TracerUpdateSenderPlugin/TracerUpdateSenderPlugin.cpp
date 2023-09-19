
#include "TracerUpdateSenderPlugin.h"


TracerUpdateSenderPlugin::TracerUpdateSenderPlugin()
{
    _pushButton = nullptr;

    _updateSenderContext = new zmq::context_t(1);
    //msgSender = new AnimHostMessageSender("127.0.0.1", false, _updateSenderContext); // It could be an idea to let the user set the IP Address using a widget

    //_updateSenderSocket = new zmq::socket_t(*_updateSenderContext, zmq::socket_type::pub);

    qDebug() << "TracerUpdateSenderPlugin created";
}

TracerUpdateSenderPlugin::~TracerUpdateSenderPlugin()
{
    if (msgSender)
        msgSender->~AnimHostMessageSender();
    //_updateSenderSocket->close();
    _updateSenderContext->close();

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
        msgSender = new AnimHostMessageSender("127.0.0.1", false, _updateSenderContext);
        zeroMQSenderThread = new QThread();

        // Example of data processing
        float numbers[] = { 10, 5,-2,0, -3.5,2.7,0.25, 2,2,2 }; // Example of sending pos/rot/scale data
        zmq::message_t* msg = new zmq::message_t(static_cast<void*>(numbers), sizeof(numbers));

        // Example of message creation and sending
        msgSender->setMessage(msg);

        msgSender->moveToThread(zeroMQSenderThread);
        QObject::connect(zeroMQSenderThread, &QThread::started, msgSender, &AnimHostMessageSender::run);

        zeroMQSenderThread->start();
        msgSender->requestStart();
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

// If the input data is set in a satisfying and coherent way, process and stream
// Maybe using widget to select a "mode" that specifies which inputs are set and how to treat them
void TracerUpdateSenderPlugin::sendAnimData() {
   /* std::string debugOut;
    float* debugDataArray = static_cast<float*>(_outMsg->data());
    for (int i = 0; i < _outMsg->size() / sizeof(float); i++) {
        debugOut = debugOut + std::to_string(debugDataArray[i]) + " ";
    }
    qDebug() << "Sending message: " << debugOut;
    _updateSenderSocket->send(*_outMsg);*/
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
