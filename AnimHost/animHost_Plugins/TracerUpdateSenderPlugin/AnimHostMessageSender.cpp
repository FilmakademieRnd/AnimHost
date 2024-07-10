/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *    
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *    
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program; 
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */

 
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
    ZMQMessageHandler::localTick->start();
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

void AnimHostMessageSender::resumeSendFrames() {
    //mutex.lock();
    _paused = false;
    reconnectWaitCondition->wakeAll();
    //mutex.unlock();
}

//void AnimHostMessageSender::setMessage(QByteArray* msg) {
//    //qDebug() << "Setting message of size " << msg->size();
//    message = msg;
//    //qDebug() << "Setting message of size " << message.size();
//
//}

void AnimHostMessageSender::run() {

    //ZMQMessageHandler::localTick->start(1000 / ZMQMessageHandler::getPlaybackFrameRate());
    //sendSocket = new zmq::socket_t(*context, zmq::socket_type::pub); // publisher socket
    sendSocket->connect(QString("tcp://127.0.0.1:5557").toLatin1().data());

    /*zmq::pollitem_t items[] = {
            { static_cast<void*>(*sendSocket), 0, ZMQ_POLLIN, 0 }
    };*/

    qDebug() << "Starting AnimHost Message Sender";// in Thread " << thread()->currentThreadId();

    //zmq::message_t* tempMsg = new zmq::message_t();

    // Allows up- and down-sampling of the animation in order to keep the perceived speed the same even though the playback framerate is not the same
    // w.r.t. the framerate, for which the animation was designed
    deltaAnimFrame = (float)ZMQMessageHandler::getAnimFrameRate() / ZMQMessageHandler::getPlaybackFrameRate();

    QByteArray* msgBodyAnim = new QByteArray();

    // The length of the animation is defined by the bones that has the more frames
    int animDataSize = 0;
    for (Bone bone : animData->mBones) {
        int boneFrames = bone.mRotationKeys.size();
        if (boneFrames > animDataSize)
            animDataSize = boneFrames;
    }

    int timestamp = INT_MIN;
    float animFrame = 0;
    bool locked = false;
    while (_working) {
        // checks if process should be aborted
        mutex.lock();
        bool stop = _stop;
        mutex.unlock();

        m_pauseMutex.lock();
        sendFrameWaitCondition->wait(&m_pauseMutex);
            
        // Send Poses Sequentially from a KEYFRAMED animation
        //qDebug() << "Timer interval" << ZMQMessageHandler::localTick->interval() << "Timer status" << ZMQMessageHandler::localTick->remainingTime();
        //qDebug() << "LocalTimeStamp = " << ZMQMessageHandler::getLocalTimeStamp();
        //qDebug() << "AnimationFrame = " << animFrame;

        SerializePose(animData, charObj, sceneNodeList, msgBodyAnim, (int) animFrame);
        //qDebug() << "animFrame =" << animFrame;
        // Create ZMQ Message
        // -> implemented in parent class because the message header is the same for all messages emitted from same client (not unique to parameter update messages)
        createNewMessage(ZMQMessageHandler::getLocalTimeStamp(), ZMQMessageHandler::MessageType::PARAMETERUPDATE, msgBodyAnim);

        // Check message data
        /*std::string debugOut;
        char* debugDataArray = message->data();
        for (int i = 0; i < message->size() / sizeof(char); i++) {
            debugOut = debugOut + std::to_string(debugDataArray[i]) + " ";
        }*/

        int type;
        size_t type_size = sizeof(type);

        sendSocket->getsockopt(ZMQ_TYPE, &type, &type_size);
        //qDebug() << "Message size: " << message->size();
        
        // Sending LOCK message to the character (necessary for applying root animations)
        if (!locked) {
            createLockMessage(ZMQMessageHandler::getLocalTimeStamp(), charObj->sceneObjectID, true);
            int retunLockVal = sendSocket->send((void*) lockMessage->data(), lockMessage->size());
            locked = true;
        }
        
        // Sending new pose message
        int retunVal = sendSocket->send((void*) message->data(), message->size());
        //qDebug() << "Attempting SEND connection on" << ZMQMessageHandler::getOwnIP();
        timestamp = ZMQMessageHandler::getLocalTimeStamp();

        if (animDataSize == 1) {    // IF   the animation data contains only one frame
            _paused = true;         // THEN stop the loop even if the loop check is marked
        }

        if (animFrame >= animDataSize && loop) {    // IF   at the end of the animation AND LOOP is checked
            animFrame = 0;                          // THEN restart streaming the animation
        } else if (animFrame < animDataSize) {      // IF   the animation has not been fully sent
            animFrame += deltaAnimFrame;            // THEN increase the frame count by the given delta (animFrameRate / playbackFrameRate)
        } else {                                    // ELSE (the animation has been fully sent AND LOOP unchecked)
            _paused = true;                            //      stop the working loop
        }
        // TODO: improving cleanup and re-connection for streaming animation again...and again...and again :)
        
        QThread::msleep(1);

        if (_paused) {
            // if _paused condition has been triggered, then wait until wakeAll signal
            // this will be triggered by clicking on the Send Animation button in the Qt pluginUI
            reconnectWaitCondition->wait(&m_pauseMutex);
            
            // after resuming, reset animFrame
            animFrame = 0;
        }
        
        m_pauseMutex.unlock();

        if (stop) {
            qDebug() << "Stopping AnimHost Message Sender";// in Thread "<<thread()->currentThreadId();
            break;
        }
        //qDebug() << "AnimHostMessageSender Running";
        //QThread::yieldCurrentThread();
    }

    createLockMessage(ZMQMessageHandler::getLocalTimeStamp(), charObj->sceneObjectID, false);
    int retunUnlockVal = sendSocket->send((void*) lockMessage->data(), lockMessage->size());
    locked = false;

    // Set _working to false -> process cannot be aborted anymore
    mutex.lock();
    _working = false;
    mutex.unlock();

    QThread::yieldCurrentThread();

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

    //  ELEMENT 0 IN THE ANIMATION DATA IS EMPTY AT THE MOMENT, SO WE CAN IGNORE THE FOLLOWING SECTION OF THE CODE
    //Bone rootBone = animData->mBones.at(1);                 // The first bone in the animation data should be the root
    
    //glm::vec3 rootPos = rootBone.GetPosition(frame);        // Getting Root Bone Position Vector
    //glm::quat rootRot = rootBone.GetOrientation(frame);     // Getting Root Bone Rotation Quaternion
    //glm::vec3 rootPos = rootBone.GetPosition(frame);        // Getting Root Bone Position Vector
    //glm::quat rootRot = rootBone.GetOrientation(frame);     // Getting Root Bone Rotation Quaternion
    //glm::vec3 rootScl = rootBone.GetScale(frame);           // Getting Root Bone Scale    Vector

    //std::vector<float> rootPosVector = { rootPos.x, rootPos.y, rootPos.z };             // converting glm::vec3 in vector<float>
    //std::vector<float> rootRotVector = { rootRot.x, rootRot.y, rootRot.z, rootRot.w };  // converting glm::quat in vector<float>
    //std::vector<float> rootSclVector = { rootScl.x, rootScl.y, rootScl.z };             // converting glm::quat in vector<float>

    //qDebug() << 1 << 1 << rootBone.mName <<rootRotVector << rootPosVector;

    // Test for sending root transform
    //rootPosVector[2] = rootPosVector[2] - (0.01 * frame);

    // Create messages for sending out the Character Root TRS and appending them to the byte array that is going to be sent to TRACER applications
    //QByteArray msgRootPos = createMessageBody(targetSceneID, character->sceneObjectID, 0, ZMQMessageHandler::ParameterType::VECTOR3,    rootPosVector);
    //byteArray->append(msgRootPos);
    //QByteArray msgRootRot = createMessageBody(targetSceneID, character->sceneObjectID, 1, ZMQMessageHandler::ParameterType::QUATERNION, rootRotVector);
    //byteArray->append(msgRootRot);
    //QByteArray msgRootScl = createMessageBody(targetSceneID, character->sceneObjectID, 2, ZMQMessageHandler::ParameterType::VECTOR3,    rootSclVector);
    //byteArray->append(msgRootScl);
    

    int nBones = character->skinnedMeshList.at(0).boneMapIDs.size();    // The number of Bones in the targeted character
    for (ushort i = 0; i < nBones; i++) {
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
        if (animDataBoneID < 0) {
            //qDebug() << boneName << "not found in the Animation Data";
            continue;
        }

        // Getting Bone Object Rotation Quaternion
        glm::quat boneQuat = animData->mBones.at(animDataBoneID).GetOrientation(frame);
        glm::vec3 bonePos  = animData->mBones.at(animDataBoneID).GetPosition(frame);  

        std::vector<float> boneQuatVector = { boneQuat.x, boneQuat.y, boneQuat.z,  boneQuat.w };    // converting glm::quat in vector<float>
        std::vector<float> bonePosVector  = { bonePos.x,  bonePos.y,  bonePos.z };                  // converting glm::vec3 in vector<float>

        //qDebug() << animDataBoneID << boneID << boneName << animData->mBones.at(animDataBoneID).mName << boneQuatVector << bonePosVector;

        QByteArray msgBoneQuat = createMessageBody(targetSceneID, character->sceneObjectID, i + 3,
                                                   ZMQMessageHandler::ParameterType::QUATERNION, boneQuatVector);
        QByteArray msgBonePos  = createMessageBody(targetSceneID, character->sceneObjectID, i + nBones + 3,
                                                   ZMQMessageHandler::ParameterType::VECTOR3,    bonePosVector);
        
        byteArray->append(msgBonePos);
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
    std::memcpy((byte*) payloadBytes, &val, getParameterDimension(parameterType));
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
    std::memcpy((byte*) payloadBytes, &val, getParameterDimension(parameterType));
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