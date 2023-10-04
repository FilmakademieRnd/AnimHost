#include "ZMQMessageHandler.h"

void ZMQMessageHandler::resume() {
    mutex.lock();
    _paused = false;
    waitCondition.wakeAll();
    mutex.unlock();
}

zmq::message_t ZMQMessageHandler::createMessage(byte targetHostID, byte time, ZMQMessageHandler::MessageType messageType,
                                            byte sceneID, byte objectID, byte parameterID, ZMQMessageHandler::ParameterType parameterType,
                                            byte* payload) {
    // Constructing new message
    QByteArray newMessage((qsizetype) 10, Qt::Uninitialized);

    // Header
    newMessage[0] = targetHostID;// Target Client ID
    newMessage[1] = time;// Time
    newMessage[2] = messageType;// Message Type

    newMessage[3] = sceneID;// Scene ID (from where do I retrieve it?)
    newMessage[4] = objectID;// Object ID (from where do I retrieve it?)
    newMessage[5] = objectID;// Object ID (from where do I retrieve it?)
    newMessage[6] = parameterID;// Parameter ID (from where do I retrieve it?)
    newMessage[7] = parameterID;// Parameter ID (from where do I retrieve it?)
    newMessage[8] = parameterType;// Parameter Type (from where do I retrieve it?)
    newMessage[9] = getParameterDimension(parameterType);            // Parameter Dimensionality (in bytes) (from where do I retrieve it?)

    newMessage.append(payload);

    zmq::message_t zmqNewMessage = zmq::message_t(static_cast<void*>(newMessage.data()), newMessage.size());

    return zmqNewMessage;
}