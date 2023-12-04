
#include "TracerUpdateSenderPlugin.h"
#include "AnimHostMessageSender.h"
#include "TickReceiver.h"


#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

TracerUpdateSenderPlugin::TracerUpdateSenderPlugin()
{
    _pushButton = nullptr;
    widget = nullptr;
    _connectIPAddress = nullptr;
    _ipAddressLayout = nullptr;
    _ipAddress = "127.0.0.1";
    _ipValidator = new QRegularExpressionValidator(ZMQMessageHandler::ipRegex, this);

    timer = new QTimer();
    timer->setTimerType(Qt::PreciseTimer);
    timer->start(0);
    _updateSenderContext = new zmq::context_t(1);

    localTime = timer->interval() % 128;

    if (!msgSender) // trying to avoid multiple instances
        msgSender = new AnimHostMessageSender(_ipAddress, false, _updateSenderContext);
    if (!zeroMQSenderThread) // trying to avoid multiple instances
        zeroMQSenderThread = new QThread();

    msgSender->moveToThread(zeroMQSenderThread);
    QObject::connect(zeroMQSenderThread, &QThread::started, msgSender, &AnimHostMessageSender::run);

    if(!tickReceiver)
        tickReceiver = new TickReceiver(this, _ipAddress, false, _updateSenderContext);
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
        return 2;
    else
        return 0;
}

NodeDataType TracerUpdateSenderPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    NodeDataType type;
    if (portType == QtNodes::PortType::In)  // INPUT Ports DataTypes
        if (portIndex == 0)
            return AnimNodeData<Animation>::staticType();
        else if (portIndex == 1)
            return AnimNodeData<CharacterPackage>::staticType();
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
            _characterIn = std::static_pointer_cast<AnimNodeData<CharacterPackage>>(data);
            break;

        default:
            return;
    }

    qDebug() << "TracerUpdateSenderPlugin setInData";
}

void TracerUpdateSenderPlugin::run() {

    qDebug() << "TracerUpdateSenderPlugin running...";

    auto sp_character = _characterIn.lock();
    auto sp_animation = _animIn.lock();
    
    // if either skeleton or animation data are NOT set and valid
    if (!sp_character || !sp_animation) {
        return;
    }

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

    std::shared_ptr<Animation> animData = _animIn.lock()->getData();
    std::shared_ptr<CharacterPackage> chpkg = _characterIn.lock()->getData();
    QByteArray* msgBodyAnim = new QByteArray();
    
    SerializeAnimation(animData, chpkg, msgBodyAnim);

    // Example of message creation
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyBool));
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyInt));
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyFloat));
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyVec3));
    //msgSender->setMessage(msgSender->createMessage(ipAddress[ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgBodyQuat));

    // create ZMQ and send it through AnimHostMessageSender
    zmq::message_t* new_msg = msgSender->createMessage(_ipAddress[_ipAddress.size() - 1].digitValue(), localTime, ZMQMessageHandler::MessageType::PARAMETERUPDATE, msgBodyAnim);
    byte* new_msg_data = (byte*) new_msg->data();
    msgSender->setMessage(new_msg);

    // This does not seem to be correct...the Thread is already started...
    if (!zeroMQSenderThread->isRunning()) {
        msgSender->requestStart();
        zeroMQSenderThread->start();
    }
}

/**
 * .
 * 
 * \param animData
 * \param byteArray
 */
void TracerUpdateSenderPlugin::SerializeAnimation(std::shared_ptr<Animation> animData, std::shared_ptr<CharacterPackage> character, QByteArray* byteArray) {
    // SceneID for testing
    std::int32_t sceneID = 0;
    
    // Character SceneObject for testing
    //std::int32_t sceneObjID = 3;
    // Bone orientation IDs for testing
    //std::int32_t parameterID[28] = { 3, 4, 5, 6, 7, 8, 28, 29, 30, 31, 9, 10, 11, 12, 51, 52, 53, 54, 47, 48, 49, 50 };
    /*if ((int) animData->mBones.size() != sizeof(parameterID)/sizeof(std::int32_t)) {
        qDebug() << "ParameterID array mismatch: " << animData->mBones.size() << " elements required, " << sizeof(parameterID)/sizeof(std::int32_t) << " provided";
        return;
    }*/
    
    for (std::int16_t i = 1; i < character->skeletonObjIDs.size(); i++) {
        // Getting Bone Object Rotation Quaternion
        glm::quat boneQuat = animData->mBones.at(i).GetOrientation(0);

        std::vector<float> boneQuatVector = { boneQuat.x, boneQuat.y, boneQuat.z,  boneQuat.w }; // converting glm::quat in vector<float>

        qDebug() << i <<animData->mBones[i].mName << boneQuatVector;

        //.......................................................................................i+2 necessary because the rotation of the first bone will have ParameterID = 3 (0 = objPos, 1 = objRot, 2 = objScale), while the order is the same
        QByteArray msgBoneQuat = msgSender->createMessageBody(sceneID, character->sceneObjectID, i+2, ZMQMessageHandler::ParameterType::QUATERNION, boneQuatVector);
        byteArray->append(msgBoneQuat);
    }
}

// When TICK is received message sender is enabled
// This Tick-Slot is connected to the Tick-Signal in the TickRecieverThread
void TracerUpdateSenderPlugin::ticked(int externalTime) {
    localTime = externalTime;
    timer->stop();
    timer->start(localTime);
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
    if (!_pushButton) {
        _pushButton = new QPushButton("Connect");
        _connectIPAddress = new QLineEdit();

        _connectIPAddress->setText(_ipAddress);
        _connectIPAddress->displayText();
        _connectIPAddress->setValidator(_ipValidator);

        _pushButton->resize(QSize(30, 30));
        _ipAddressLayout = new QHBoxLayout();

        _ipAddressLayout->addWidget(_connectIPAddress);
        _ipAddressLayout->addWidget(_pushButton);

        _ipAddressLayout->setSizeConstraint(QLayout::SetMinimumSize);

        widget = new QWidget();

        widget->setLayout(_ipAddressLayout);
        connect(_pushButton, &QPushButton::released, this, &TracerUpdateSenderPlugin::onButtonClicked);
    }

    widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
                          "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
                          "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
                          "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
    );

    return widget;
}

void TracerUpdateSenderPlugin::onButtonClicked() {
    // Open ZMQ port to receive the scene
    _ipAddress = _connectIPAddress->text();
    tickReceiver->setIPAddress(_ipAddress);

    tickReceiver->requestStart();
    zeroMQTickReceiverThread->start();

    qDebug() << "Attempting SEND connection to" << _ipAddress;
}
