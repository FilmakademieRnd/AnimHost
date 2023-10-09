#include "ZMQMessageHandler.h"

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
    qDebug() << "Serialize vector " << type;

    switch (type) {
        case ZMQMessageHandler::VECTOR2: {
            qDebug() << "Vector2";
            std::vector<float> vec2 = std::any_cast<std::vector<float>>(_vector);
            float x = vec2[0];
            float y = vec2[1];

            qDebug() << "[" + std::to_string(x) + ", " + std::to_string(y) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &x, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &y, sizeof(float));
            break;
        }
        case ZMQMessageHandler::VECTOR3: {
            qDebug() << "Vector3";
            std::vector<float> vec3 = std::any_cast<std::vector<float>>(_vector);
            float x = vec3[0];
            float y = vec3[1];
            float z = vec3[2];

            qDebug() << "[" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &x, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &y, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &z, sizeof(float));

            break;
        }
        case ZMQMessageHandler::VECTOR4: {
            qDebug() << "Vector4";
            std::vector<float> vec4 = std::any_cast<std::vector<float>>(_vector);
            float x = vec4[0];
            float y = vec4[1];
            float z = vec4[2];
            float w = vec4[3];
            
            qDebug() << "[" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &x, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &y, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &z, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &w, sizeof(float));

            break;
        }
        case ZMQMessageHandler::QUATERNION: {
            qDebug() << "Quaternion";
            std::vector<float> quat = std::any_cast<std::vector<float>>(_vector);
            float x = -quat[0];
            float y = quat[1];
            float z = quat[2];
            float w = quat[3];
            
            qDebug() << "[" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &x, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &y, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &z, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &w, sizeof(float));

            break;
        }
        case ZMQMessageHandler::COLOR: {
            qDebug() << "Colour";
            std::vector<float> color = std::any_cast<std::vector<float>>(_vector);
            float r = color[0];
            float g = color[1];
            float b = color[2];
            float a = color[3];
            
            qDebug() << "[" + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a) + "]";

            byte offset = 0;
            std::memcpy(dest + offset, &r, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &g, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &b, sizeof(float)); offset += sizeof(float);
            std::memcpy(dest + offset, &a, sizeof(float));

            break;
        }
    }
}

zmq::message_t* ZMQMessageHandler::createMessage(byte targetHostID, byte time, ZMQMessageHandler::MessageType messageType,
                                            byte sceneID, byte objectID, byte parameterID, ZMQMessageHandler::ParameterType parameterType,
                                            bool payload) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }

    qDebug() << "Serialize bool: " << payload;

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

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    bool val = payload;
    std::memcpy((byte*) payloadBytes, &val, sizeof(bool));
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    bool _bool = *(bool*) payloadBytes;
    debugOut = std::to_string(_bool);
    qDebug() << "Payload data: " + debugOut;

    void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;
}

zmq::message_t* ZMQMessageHandler::createMessage(byte targetHostID, byte time, ZMQMessageHandler::MessageType messageType,
                                                 byte sceneID, byte objectID, byte parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                 std::int32_t payload) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }

    qDebug() << "Serialize int: " << payload;

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

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    std::int32_t val = payload;
    std::memcpy((byte*) payloadBytes, &val, sizeof(std::int32_t));
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    std::int32_t _int = *(std::int32_t*) payloadBytes;
    debugOut = std::to_string(_int);
    qDebug() << "Payload data: " + debugOut;

    void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;
}

zmq::message_t* ZMQMessageHandler::createMessage(byte targetHostID, byte time, ZMQMessageHandler::MessageType messageType,
                                                 byte sceneID, byte objectID, byte parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                 float payload) {
    try {
        if (targetHostID == -1)
            throw (targetHostID);
    } catch (int targetHostID) {
        qDebug() << "Invalid target host ID";
    }
    qDebug() << "Serialize float: " << std::to_string(payload);

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

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    float val = payload;
    std::memcpy((byte*) payloadBytes, &val, sizeof(float));
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    float _float = *(float*) payloadBytes;
    debugOut = std::to_string(_float);
    qDebug() << "Payload data: " + debugOut;

    void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;
}

zmq::message_t* ZMQMessageHandler::createMessage(byte targetHostID, byte time, ZMQMessageHandler::MessageType messageType,
                                                 byte sceneID, byte objectID, byte parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                 std::vector<float> payload) {
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

    const char* payloadBytes = (char*)malloc(getParameterDimension(parameterType));
    SerializeVector((byte *)payloadBytes, payload, parameterType);
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    for (int i = 0; i < 3; i++) {
        int index = sizeof(float) * i;

        float _float = *(float*) (payloadBytes + index);

        debugOut = debugOut + std::to_string(_float) + " ";
    }
    qDebug() << "Payload data: " + debugOut;

    void* msgData = newMessage.data();
    size_t msgSize = newMessage.size();
    zmq::message_t* zmqNewMessage = new zmq::message_t(msgData, msgSize);

    return zmqNewMessage;
}