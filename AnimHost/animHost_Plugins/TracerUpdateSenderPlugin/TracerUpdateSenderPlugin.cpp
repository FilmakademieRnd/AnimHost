
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

    localTime = timer->interval() % 128;

    zeroMQTickReceiverThread = new QThread();
    tickReceiver = new TickReceiver(this, ipAddress, false, _updateSenderContext);

    tickReceiver->moveToThread(zeroMQTickReceiverThread);
    QObject::connect(tickReceiver, &TickReceiver::tick, this, &TracerUpdateSenderPlugin::ticked);
    QObject::connect(zeroMQTickReceiverThread, &QThread::started, tickReceiver, &TickReceiver::run);

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

    tickReceiver->requestStart();
    zeroMQTickReceiverThread->start();

    msgSender = new AnimHostMessageSender(ipAddress, false, _updateSenderContext);
    zeroMQSenderThread = new QThread();

    /*bool boolExample = true;
    QByteArray msgBodyBool = msgSender->createMessageBody(0, 0, 0, ZMQMessageHandler::ParameterType::BOOL, boolExample);*/

    /*int intExample = -64;
    QByteArray msgBodyInt = msgSender->createMessageBody(0, 0, 0, ZMQMessageHandler::ParameterType::INT, intExample);*/
    
    /*float floatExample = 78.3;
    QByteArray msgBodyFloat = msgSender->createMessageBody(0, 0, 0, ZMQMessageHandler::ParameterType::FLOAT, floatExample);*/

    //std::vector<float> vec3Example = { 2.5, -7.4, 0 }; // To be substituted with REAL DATA (from sp_akeleton and sp_animation)
    //QByteArray msgBodyVec3 = msgSender->createMessageBody(0, 0, 0, ZMQMessageHandler::ParameterType::VECTOR3, vec3Example);

    std::shared_ptr<AnimNodeData<Animation>> animNodeData = std::shared_ptr<AnimNodeData<Animation>>(_animIn);
    std::shared_ptr<Animation> animData = animNodeData->getData();
    QByteArray* msgBodyAnim = new QByteArray();
    SerializeAnimation(animData, msgBodyAnim);

    // Example of message creation and 
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyBool));
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyInt);
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyFloat);
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyVec3);

    // create ZMQ and send it through AnimHostMessageSender
    zmq::message_t* new_msg = msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, msgBodyAnim);
    byte* new_msg_data = (byte*) new_msg->data();
    msgSender->setMessage(new_msg);

    msgSender->moveToThread(zeroMQSenderThread);
    QObject::connect(zeroMQSenderThread, &QThread::started, msgSender, &AnimHostMessageSender::run);

    msgSender->requestStart();
    zeroMQSenderThread->start();
}

void TracerUpdateSenderPlugin::SerializeAnimation(std::shared_ptr<Animation> animData, QByteArray* byteArray) {
    for (std::int16_t i = 0; i < animData->mBones.size(); i++) {
        // Getting Bone Object ID
        std::int16_t boneID = 0;
        boneID = animData->mBones[i].mID;
        //qDebug() << "Bone ID: " << boneID;

        // Getting Bone Object Rotation Quaternion
        glm::quat boneQuat = animData->mBones[i].GetOrientation(0);
        std::vector<float> boneQuatVector = { boneQuat.x, boneQuat.y, boneQuat.z, boneQuat.w }; // converting glm::quat in vector<float>

        // How do I retrieve sceneID? Where is ParameterID placed?
        QByteArray msgBoneQuat = msgSender->createMessageBody(0, i, 0, ZMQMessageHandler::ParameterType::QUATERNION, boneQuatVector);
        byteArray->append(msgBoneQuat);
    }
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
