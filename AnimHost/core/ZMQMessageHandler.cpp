#include "ZMQMessageHandler.h"

// Defining and initialising static members

QWaitCondition* ZMQMessageHandler::sendFrameWaitCondition = new QWaitCondition();
QWaitCondition* ZMQMessageHandler::reconnectWaitCondition = new QWaitCondition();

byte ZMQMessageHandler::targetSceneID = 0;
QString ZMQMessageHandler::ownIP = "";
QList<QHostAddress> ZMQMessageHandler::ipList;

QTimer* ZMQMessageHandler::localTick = new QTimer();
int ZMQMessageHandler::localTimeStamp = 0;
int ZMQMessageHandler::bufferSize = 120;
int ZMQMessageHandler::animFrameRate = 30;
int ZMQMessageHandler::playbackFrameRate = 60;

ZMQMessageHandler::ZMQMessageHandler() {
    ZMQMessageHandler::ipList = QNetworkInterface::allAddresses();
    ZMQMessageHandler::ipList.removeIf([] (QHostAddress ipAddress) { return ipAddress.protocol() == 1; }); // remove entry if protocol is IPv6

    ZMQMessageHandler::localTick->setTimerType(Qt::PreciseTimer);
    ZMQMessageHandler::localTick->setSingleShot(false);
    ZMQMessageHandler::localTick->setInterval(1000/ZMQMessageHandler::playbackFrameRate);
    QObject::connect(ZMQMessageHandler::localTick, &QTimer::timeout, this, &ZMQMessageHandler::increaseTimeStamp);

    //debug
    for (QHostAddress ipAddress : ZMQMessageHandler::ipList) {
        qDebug() << ipAddress.toString();
    }
}

//void ZMQMessageHandler::Serialize(byte* dest, bool _value) {
//    bool val = std::any_cast<bool>(_value);
//    std::memcpy(dest, &val, sizeof(bool));
//}
//
//void ZMQMessageHandler::Serialize(byte* dest, int _value) {
//    __int32 val = std::any_cast<__int32>(_value);
//    std::memcpy(dest, &val, sizeof(__int32));
//}
//
//void ZMQMessageHandler::Serialize(byte* dest, float _value) {
//    float val = std::any_cast<float>(_value);
//    std::memcpy(dest, &val, sizeof(float));
//}

// Creating ZMQ Message from existing QByteArray
void ZMQMessageHandler::createNewMessage(byte timestamp, ZMQMessageHandler::MessageType messageType, QByteArray* body) {
    /*try {
        byte targetHostID = getTargetHostID();
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }*/

    qDebug() << "Creating ZMQ Message from existing QByteArray. OwnID =" << ZMQMessageHandler::getOwnID() << "and time" << timestamp;

    // Constructing new message
    message->clear();
    
    // Header
    message->insert(0, ZMQMessageHandler::getOwnID());  // OwnID 
    message->insert(1, timestamp);                      // Time
    message->insert(2, messageType);                    // Message Type

    message->append(*body);
}