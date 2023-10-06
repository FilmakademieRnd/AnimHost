#include "ZMQMessageHandler.h"

void ZMQMessageHandler::resume() {
    mutex.lock();
    _paused = false;
    waitCondition.wakeAll();
    mutex.unlock();
}

//void ZMQMessageHandler::Serialize(byte* dest, bool data) {
//   
//}

//void ZMQMessageHandler::Serialize(byte* dest, int data) {
//   
//}

//void ZMQMessageHandler::Serialize(byte* dest, float data) {
//   
//}

//void ZMQMessageHandler::Serialize(byte* dest, float[] data, ZMQMessageHandler::ParameterType type) {
//   
//}

//void ZMQMessageHandler::Serialize(byte* dest, std::string data) {
//   
//}

zmq::message_t* ZMQMessageHandler::createMessage(byte targetHostID, byte time, ZMQMessageHandler::MessageType messageType,
                                            byte sceneID, byte objectID, byte parameterID, ZMQMessageHandler::ParameterType parameterType,
                                            byte* payload, size_t payloadSize) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }

    // Constructing new message
    QByteArray newMessage((qsizetype) 10, Qt::Uninitialized);

    // Header
    newMessage[0] = targetHostID;                           // Target Client ID
    newMessage[1] = time;                                   // Time
    newMessage[2] = messageType;                            // Message Type

    newMessage[3] = sceneID;                                // Scene ID (from where do I retrieve it?)
    newMessage[4] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[5] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[6] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[7] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[8] = parameterType;                          // Parameter Type (from where do I retrieve it?)
    newMessage[9] = getParameterDimension(parameterType);   // Parameter Dimensionality (in bytes) (from where do I retrieve it?)

    QByteArray payloadQByteArray = QByteArray::fromRawData((char *)(payload), payloadSize);
    newMessage.append(payloadQByteArray);

    void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;
}