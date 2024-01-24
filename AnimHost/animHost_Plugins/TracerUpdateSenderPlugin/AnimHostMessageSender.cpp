#include "AnimHostMessageSender.h"

//targetHostID = _ipAddress.at(_ipAddress.size() - 1).digitValue();

#include <QThread>
#include <QDebug>
#include <iostream>

void AnimHostMessageSender::requestStart() {
    mutex.lock();
    _working = true;
    _stop = false;
    _paused = false;
    qDebug() << "AnimHost Message Sender requested to start";// in Thread "<<thread()->currentThreadId();

    sendSocket = new zmq::socket_t(*context, zmq::socket_type::pub); // publisher socket
    mutex.unlock();
}

void AnimHostMessageSender::requestStop() {
    mutex.lock();
    if (_working) {
        _stop = true;
        _paused = false;
        //_working = false;
        qDebug() << "AnimHost Message Sender stopping";// in Thread "<<thread()->currentThreadId();
    }
    mutex.unlock();
}

//void AnimHostMessageSender::setMessage(QByteArray* msg) {
//    //qDebug() << "Setting message of size " << msg->size();
//    message = msg;
//    //qDebug() << "Setting message of size " << message.size();
//
//}

void AnimHostMessageSender::run() {

    //sendSocket = new zmq::socket_t(*context, zmq::socket_type::pub); // publisher socket
    sendSocket->connect(QString("tcp://127.0.0.1:5557").toLatin1().data());

    /*zmq::pollitem_t items[] = {
            { static_cast<void*>(*sendSocket), 0, ZMQ_POLLIN, 0 }
    };*/

    qDebug() << "Starting AnimHost Message Sender";// in Thread " << thread()->currentThreadId();

    //zmq::message_t* tempMsg = new zmq::message_t();
    int type;
    size_t type_size = sizeof(type);
    int count = 0;
    while (_working) {

        // checks if process should be aborted
        mutex.lock();
        bool stop = _stop;
        if (_paused)
            waitCondition.wait(&mutex); // in this place, your thread will stop to execute until someone calls resume
        else
            _paused = true; // Execute once and pause at the beginning of next iteration of while-loop
        mutex.unlock();

        bool msgIsExternal = false;

        QByteArray* msgBodyAnim = new QByteArray();

        // The length of the animation is defined by the bones that has the more frames
        int animDataSize = 0;
        for (Bone bone : animData->mBones) {
            int boneFrames = bone.mRotationKeys.size();
            if (boneFrames > animDataSize)
                animDataSize = boneFrames;
        }

        // Define duration single frame (default 17 = ~60fps) - [40 = 25fps] - [42 = ~24fps] - [33 = ~30fps]
        int frameDurationMillisec = 17;
        // In case the animation has a valid duration metadata, computing the duration of each frame
        if (animData->mDuration > 0) {
            frameDurationMillisec = (animData->mDuration * 1000) / animData->mDurationFrames;
        }

        // Send Poses Sequentially
        for (int frame = 0; frame < animDataSize; frame++) {
            // At the moment, sleeping is not required nor wanted because we want to fill the ring buffer of the receiver faster then it is consumed
            QThread::msleep(1);

            SerializePose(animData, charObj, sceneNodeList, msgBodyAnim, frame);

            byte timestamp = frame % 120;

            // create ZMQ Message
            // implemented in parent class because the message header is the same for all messages emitted from same client (not unique to parameter update messages)
            createNewMessage(timestamp, ZMQMessageHandler::MessageType::PARAMETERUPDATE, msgBodyAnim);

            // Check message data
            std::string debugOut;
            char* debugDataArray = message->data();
            for (int i = 0; i < message->size() / sizeof(char); i++) {
                debugOut = debugOut + std::to_string(debugDataArray[i]) + " ";
            }

            sendSocket->getsockopt(ZMQ_TYPE, &type, &type_size);

            qDebug() << "Message size: " << message->size();

            // Sending message
            int retunVal = sendSocket->send((void*) message->data(), message->size());
            qDebug() << "Attempting SEND connection on" << ZMQMessageHandler::getOwnIP();

            QThread::msleep(1);

            // if loop enabled reset frame to 0 at the end of the animation (except when the animation has only one frame)
            if (loop && frame > 0 && frame == animDataSize - 1)
                frame = 0;
        }

        if (stop) {
            qDebug() << "Stopping AnimHost Message Sender";// in Thread "<<thread()->currentThreadId();
            break;
        }
        QThread::yieldCurrentThread();
    }

    // Set _working to false -> process cannot be aborted anymore
    mutex.lock();
    _working = false;
    mutex.unlock();

    qDebug() << "AnimHost Message Sender process stopped";// in Thread "<<thread()->currentThreadId();

    emit stopped();
}

void AnimHostMessageSender::setAnimationAndSceneData(std::shared_ptr<Animation> ad, std::shared_ptr<CharacterObject> co, std::shared_ptr<SceneNodeObjectSequence> snl) {
    animData = ad;
    charObj = co;
    sceneNodeList = snl;
}

/**
 * .
 *
 * \param animData
 * \param byteArray
 */
void AnimHostMessageSender::SerializePose(std::shared_ptr<Animation> animData, std::shared_ptr<CharacterObject> character,
                                             std::shared_ptr<SceneNodeObjectSequence> sceneNodeList, QByteArray* byteArray, int frame) {
    //Overwrite byteArray
    byteArray->clear();

    // Target Scene ID
    int targetSceneID = ZMQMessageHandler::getTargetSceneID();

    // Character SceneObject for testing
    //std::int32_t sceneObjID = 3;
    // Bone orientation IDs for testing
    //std::int32_t parameterID[28] = { 3, 4, 5, 6, 7, 8, 28, 29, 30, 31, 9, 10, 11, 12, 51, 52, 53, 54, 47, 48, 49, 50 };
    /*if ((int) animData->mBones.size() != sizeof(parameterID)/sizeof(std::int32_t)) {
        qDebug() << "ParameterID array mismatch: " << animData->mBones.size() << " elements required, " << sizeof(parameterID)/sizeof(std::int32_t) << " provided";
        return;
    }*/

    int rootBoneID = character->skinnedMeshList.at(0).boneMapIDs.at(0);
    for (ushort i = 0; i < character->skinnedMeshList.at(0).boneMapIDs.size(); i++) {
        // boneMapIDs contains the parameterID to boneID mapping
        //      parameterID (= i+3)                 id to be sent in the update message to the rendering application, the offset (+3) is necessary because the first 3 parameters are ALWAYS rootPos, rootRot, rootScl
        //      boneID      (= boneMapIDs.at(i))    id to be used to get BONE NAME given the list of SceneNodes in the received scene
        //      boneName                            name to be used to get BONE QUATERNION from the animation data
        // This WILL NOT WORK for RETARGETED animations

        // Getting boneName given the parameterID
        int boneID = character->skinnedMeshList.at(0).boneMapIDs.at(i);
        std::string boneName = sceneNodeList->mSceneNodeObjectSequence.at(boneID).objectName;
        // boneName search (sequential...any ideas on how to make it faster?)
        int animDataBoneID = -1;
        for (int j = 0; j < animData->mBones.size(); j++) {
            if (boneName.compare(animData->mBones.at(j).mName) == 0) {
                animDataBoneID = j;
                break;
            }
        }
        assert(animDataBoneID >= 0);

        Bone selectedBone = animData->mBones.at(animDataBoneID);

        // Getting Bone Object Rotation Quaternion
        glm::quat boneQuat = selectedBone.GetOrientation(frame);

        std::vector<float> boneQuatVector = { boneQuat.x, boneQuat.y, boneQuat.z,  boneQuat.w }; // converting glm::quat in vector<float>

        qDebug() << i << boneID << boneName << animData->mBones.at(animDataBoneID).mName << boneQuatVector;

        QByteArray msgBoneQuat = createMessageBody(targetSceneID, character->sceneObjectID, i + 3, ZMQMessageHandler::ParameterType::QUATERNION, boneQuatVector);
        byteArray->append(msgBoneQuat);
    }
}

// Creating ZMQ Parameter Update Message Body from bool value
QByteArray AnimHostMessageSender::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                bool payload) {
    //qDebug() << "Serialize bool: " << payload;
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
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+1)

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    assert(payloadBytes != NULL);
    bool val = payload;
    std::memcpy((byte*) payloadBytes, &val, sizeof(bool));
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    bool _bool = *(bool*) payloadBytes;
    debugOut = std::to_string(_bool);

    return newMessage;
}

// Creating ZMQ Parameter Update Message Body from 32-bit int
QByteArray AnimHostMessageSender::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                std::int32_t payload) {
    //qDebug() << "Serialize int: " << payload;
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
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+4)

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    assert(payloadBytes != NULL);
    std::int32_t val = payload;
    std::memcpy((byte*) payloadBytes, &val, sizeof(std::int32_t));
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    std::int32_t _int = *(std::int32_t*) payloadBytes;
    debugOut = std::to_string(_int);
    //qDebug() << "Payload data: " + debugOut;

    return newMessage;
}

// Creating ZMQ Parameter Update Message Body from float value
QByteArray AnimHostMessageSender::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                float payload) {
    //qDebug() << "Serialize float: " << std::to_string(payload);
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
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+4)

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    assert(payloadBytes != NULL);
    float val = payload;
    std::memcpy((byte*) payloadBytes, &val, sizeof(float));
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    float _float = *(float*) payloadBytes;
    debugOut = std::to_string(_float);
    qDebug() << "Payload data: " + debugOut;

    return newMessage;
}

// Creating ZMQ Parameter Update Message Body from string value
QByteArray AnimHostMessageSender::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                std::string payload) {
    //qDebug() << "Serialize string: " << payload;
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
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+var)

    payload.shrink_to_fit();
    const char* payloadBytes = (char*) malloc(payload.size());
    assert(payloadBytes != NULL);
    std::memcpy((byte*) payloadBytes, &payload, payload.size());
    newMessage.append(payloadBytes, (qsizetype) payload.size());

    std::string debugOut = std::any_cast<std::string>(payloadBytes);
    //qDebug() << "Payload data: " + debugOut;

    return newMessage;
}


// Creating ZMQ Parameter Update Message Body from float vector (VECTOR2-3-4, QUATERNION, COLOR)
QByteArray AnimHostMessageSender::createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                                std::vector<float> payload) {
    byte targetSceneID;
    try {
        targetSceneID = getTargetSceneID();
        if (targetSceneID == -1)
            throw (targetSceneID);
    } catch (int targetSceneID) {
        qDebug() << "Invalid target scene ID";
    }
    //qDebug() << "Serialize float vector: " << std::to_string(payload);
    byte objID_1 = (byte) (objectID & 0xFF);          // Masking 8 highest bits -> extracting lowest 8 bits
    byte objID_2 = (byte) ((objectID >> 8) & 0xFF);   // Shifting 8 bits to the right -> extracting highest 8 bits
    byte parID_1 = (byte) (parameterID & 0xFF);          // Masking 8 highest bits -> extracting lowest 8 bits
    byte parID_2 = (byte) ((parameterID >> 8) & 0xFF);   // Shifting 8 bits to the right -> extracting highest 8 bits

    // Constructing new message
    QByteArray newMessage((qsizetype) 7, Qt::Uninitialized);

    newMessage[0] = targetSceneID;                              // Scene ID
    newMessage[1] = objID_1;                                    // Object ID byte 1
    newMessage[2] = objID_2;                                    // Object ID byte 2 
    newMessage[3] = parID_1;                                    // Parameter ID
    newMessage[4] = parID_2;                                    // Parameter ID
    newMessage[5] = parameterType;                              // Parameter Type
    newMessage[6] = getParameterDimension(parameterType) + 7;   // Parameter Message Dimensionality (in bytes) - i.e. size of the param. HEADER + VALUES (7+8/12/16)

    const char* payloadBytes = (char*) malloc(getParameterDimension(parameterType));
    assert(payloadBytes != NULL);
    SerializeVector((byte*) payloadBytes, payload, parameterType);
    newMessage.append(payloadBytes, (qsizetype) getParameterDimension(parameterType));

    std::string debugOut;
    for (int i = 0; i < 3; i++) {
        int index = sizeof(float) * i;

        float _float = *(float*) (payloadBytes + index);

        debugOut = debugOut + std::to_string(_float) + " ";
    }
    //qDebug() << "Payload data: " + debugOut;

    return newMessage;
}

void AnimHostMessageSender::SerializeVector(byte* dest, std::vector<float> _vector, ZMQMessageHandler::ParameterType type) {
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