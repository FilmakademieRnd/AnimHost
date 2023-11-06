#include "ZMQMessageHandler.h"

ZMQMessageHandler::ZMQMessageHandler() {
    
}

void ZMQMessageHandler::resume() {
    mutex.lock();
    _paused = false;
    waitCondition.wakeAll();
    mutex.unlock();
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

void ZMQMessageHandler::SerializeVector(byte* dest, std::vector<float> _vector, ZMQMessageHandler::ParameterType type) {
    //qDebug() << "Serialize vector " << type;

    switch (type) {
        case ZMQMessageHandler::VECTOR2: {
            //qDebug() << "Vector2";
            std::vector<float> vec2 = std::any_cast<std::vector<float>>(_vector);
            float x = vec2[0];
            float y = vec2[1];

            //qDebug() << "[" + std::to_string(x) + ", " + std::to_string(y) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &x, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &y, sizeof(float));
            break;
        }
        case ZMQMessageHandler::VECTOR3: {
            //qDebug() << "Vector3";
            std::vector<float> vec3 = std::any_cast<std::vector<float>>(_vector);
            float x = vec3[0];
            float y = vec3[1];
            float z = vec3[2];

            //qDebug() << "[" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &x, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &y, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &z, sizeof(float));

            break;
        }
        case ZMQMessageHandler::VECTOR4: {
            //qDebug() << "Vector4";
            std::vector<float> vec4 = std::any_cast<std::vector<float>>(_vector);
            float x = vec4[0];
            float y = vec4[1];
            float z = vec4[2];
            float w = vec4[3];
            
            //qDebug() << "[" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &x, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &y, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &z, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &w, sizeof(float));

            break;
        }
        case ZMQMessageHandler::QUATERNION: {
            //qDebug() << "Quaternion";
            std::vector<float> quat = std::any_cast<std::vector<float>>(_vector);
            float x = quat[0];
            float y = quat[1];
            float z = quat[2];
            float w = quat[3];
            
            //qDebug() << "[" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &x, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &y, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &z, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &w, sizeof(float));

            break;
        }
        case ZMQMessageHandler::COLOR: {
            //qDebug() << "Colour";
            std::vector<float> color = std::any_cast<std::vector<float>>(_vector);
            float r = color[0];
            float g = color[1];
            float b = color[2];
            float a = color[3];
            
            //qDebug() << "[" + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &r, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &g, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &b, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &a, sizeof(float));

            break;
        }
    }
}


// Creating ZMQ Message from existing QByteArray
zmq::message_t* ZMQMessageHandler::createMessage(byte targetHostID, byte time, ZMQMessageHandler::MessageType messageType, QByteArray* body) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }

    qDebug() << "Creating ZMQ Message from existing QByteArray";

    // Constructing new message
    QByteArray newMessage((qsizetype) 3, Qt::Uninitialized);

    // Header
    newMessage[0] = targetHostID;                           // Target Client ID
    newMessage[1] = time;                                   // Time
    newMessage[2] = messageType;                            // Message Type

    newMessage.append(*body);

    const void* msgData = newMessage.constData();
    //const byte* msgDataByte = (byte*)newMessage.constData();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);
    byte* newMsgData = (byte*)zmqNewMessage->data(); // just for debug

    return zmqNewMessage;
}

// Creating ZMQ Message Body from bool value
QByteArray ZMQMessageHandler::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                bool payload) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }

    //qDebug() << "Serialize bool: " << payload;

    // Constructing new message
    QByteArray newMessage((qsizetype) 7, Qt::Uninitialized);

    newMessage[0] = sceneID;                                // Scene ID (from where do I retrieve it?)
    newMessage[1] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[2] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[3] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[4] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[5] = parameterType;                          // Parameter Type (from where do I retrieve it?)
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+1)

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    bool val = payload;
    std::memcpy((byte*) payloadBytes, &val, sizeof(bool));
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    bool _bool = *(bool*) payloadBytes;
    debugOut = std::to_string(_bool);
    //qDebug() << "Payload data: " + debugOut;

    return newMessage;
    /*void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;*/
}

// Creating ZMQ Message Body from 32-bit int
QByteArray ZMQMessageHandler::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                std::int32_t payload) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }

    //qDebug() << "Serialize int: " << payload;

    // Constructing new message
    QByteArray newMessage((qsizetype) 7, Qt::Uninitialized);

    newMessage[0] = sceneID;                                // Scene ID (from where do I retrieve it?)
    newMessage[1] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[2] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[3] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[4] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[5] = parameterType;                          // Parameter Type (from where do I retrieve it?)
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+4)

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    std::int32_t val = payload;
    std::memcpy((byte*) payloadBytes, &val, sizeof(std::int32_t));
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    std::int32_t _int = *(std::int32_t*) payloadBytes;
    debugOut = std::to_string(_int);
    //qDebug() << "Payload data: " + debugOut;

    return newMessage;
    /*void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;*/
}

// Creating ZMQ Message Body from float value
QByteArray ZMQMessageHandler::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                float payload) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }
    //qDebug() << "Serialize float: " << std::to_string(payload);

    // Constructing new message
    QByteArray newMessage((qsizetype) 7, Qt::Uninitialized);

    newMessage[0] = sceneID;                                // Scene ID (from where do I retrieve it?)
    newMessage[1] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[2] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[3] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[4] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[5] = parameterType;                          // Parameter Type (from where do I retrieve it?)
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+4)

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    float val = payload;
    std::memcpy((byte*) payloadBytes, &val, sizeof(float));
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    float _float = *(float*) payloadBytes;
    debugOut = std::to_string(_float);
    qDebug() << "Payload data: " + debugOut;

    return newMessage;
    /*void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;*/
}

// Creating ZMQ Message Body from string value
QByteArray ZMQMessageHandler::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                std::string payload) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }
    //qDebug() << "Serialize serialize: " << payload;

    // Constructing new message
    QByteArray newMessage((qsizetype) 7, Qt::Uninitialized);

    newMessage[0] = sceneID;                                // Scene ID (from where do I retrieve it?)
    newMessage[1] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[2] = objectID;                               // Object ID (from where do I retrieve it?)
    newMessage[3] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[4] = parameterID;                            // Parameter ID (from where do I retrieve it?)
    newMessage[5] = parameterType;                          // Parameter Type (from where do I retrieve it?)
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+variable)

    payload.shrink_to_fit();
    const char* payloadBytes = (char*) malloc(payload.size());
    std::memcpy((byte*) payloadBytes, &payload, payload.size());
    newMessage.append(payloadBytes, (qsizetype) payload.size());

    std::string debugOut = std::any_cast<std::string>(payloadBytes);
    //qDebug() << "Payload data: " + debugOut;

    return newMessage;
    /*void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;*/
}


// Creating ZMQ Message Body from float vector (VECTOR2-3-4, QUATERNION, COLOR)
QByteArray ZMQMessageHandler::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                std::vector<float> payload) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }

    byte objID_1 = (byte) (objectID & 0xFF);          // Masking 8 highest bits -> extracting lowest 8 bits
    byte objID_2 = (byte) ((objectID >> 8) & 0xFF);   // Shifting 8 bits to the right -> extracting highest 8 bits
    byte parID_1 = (byte) (parameterID & 0xFF);          // Masking 8 highest bits -> extracting lowest 8 bits
    byte parID_2 = (byte) ((parameterID >> 8) & 0xFF);   // Shifting 8 bits to the right -> extracting highest 8 bits

    // Constructing new message
    QByteArray newMessage((qsizetype) 7, Qt::Uninitialized);

    newMessage[0] = sceneID;                                    // Scene ID
    newMessage[1] = objID_1;                                    // Object ID byte 1
    newMessage[2] = objID_2;                                    // Object ID byte 2 
    newMessage[3] = parID_1;                                    // Parameter ID
    newMessage[4] = parID_2;                                    // Parameter ID
    newMessage[5] = parameterType;                              // Parameter Type
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+8/12/16)

    const char* payloadBytes = (char*)malloc(getParameterDimension(parameterType));
    SerializeVector((byte *)payloadBytes, payload, parameterType);
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    for (int i = 0; i < 3; i++) {
        int index = sizeof(float) * i;

        float _float = *(float*) (payloadBytes + index);

        debugOut = debugOut + std::to_string(_float) + " ";
    }
    //qDebug() << "Payload data: " + debugOut;

    return newMessage;
    /*void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;*/
}