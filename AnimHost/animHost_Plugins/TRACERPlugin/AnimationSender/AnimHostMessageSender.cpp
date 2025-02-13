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

#include <QThread>
#include <QDebug>
#include <QDataStream>
#include <QElapsedTimer>
#include <iostream>

void AnimHostMessageSender::requestStart() {
    mutex.lock();
    _working = true;
    _stop = false;
    _paused = false;

    qDebug() << "AnimHost Message Sender requested to start";

    sendSocket = new zmq::socket_t(*context, zmq::socket_type::pub); // publisher socket
    mutex.unlock();
}

void AnimHostMessageSender::requestStop() {
    mutex.lock();
    if (_working) {
        _stop = true;
        _paused = false;
        _working = false;
        qDebug() << "AnimHost Message Sender stopping";
    }
    mutex.unlock();
}

void AnimHostMessageSender::resumeSendFrames() {
    //mutex.lock();
    _paused = false;
    reconnectWaitCondition->wakeAll();
    //mutex.unlock();
}

void AnimHostMessageSender::run() {

    sendSocket->connect(QString("tcp://" + _targetIP + ":5557").toLatin1().data());

    while (!_stop) { //loop until stop is requested (by calling requestStop(); on deletion of sender node in compute graph)

        
        if (targetAddressChanged) { //check if IP address has changed & reconnect
            qDebug() << "Target IP Address changed to: " << _targetIP;
            qDebug() << "Try opening socket to: " << _targetIP;
            sendSocket->close();
            sendSocket = new zmq::socket_t(*context, zmq::socket_type::pub);
            sendSocket->connect(QString("tcp://" + _targetIP + ":5557").toLatin1().data());
            targetAddressChanged = false;

            qDebug() << "Connected to: " << _targetIP;
        }

        if (streamAnimation) {  
            			
            streamAnimationData(); // on completion of streaming the animation, the streamAnimation flag is set to false

        }
        else if (!streamAnimation && sendBlock) {

            sendAnimationDataBlock(); // on completion of sending the animation block, the sendBlock flag is set to false

        }


    };

	sendSocket->close();

    qDebug() << "AnimHost Message Sender process to be stopped";

    emit stopped();
}


void AnimHostMessageSender::streamAnimationData()
{
    qDebug() << "Starting STREAM AnimHost Message Sender";

    //zmq::message_t* tempMsg = new zmq::message_t();

    // Allows up- and down-sampling of the animation in order to keep the perceived speed the same even though the playback framerate is not the same
    // w.r.t. the framerate, for which the animation was designed
    deltaAnimFrame = 1.f; //_globalTimer->getAnimFrameRate() / _globalTimer->getPlaybackFrameRate();

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
    while (_working && streamAnimation) {
        // checks if process should be aborted
        mutex.lock();
        bool stop = _stop;
        mutex.unlock();

        if (stop) {
			break;
		}

        m_pauseMutex.lock();


        // Wait for TRACER Tick to send the next frame
        _globalTimer->waitOnTick();
  

        // Send Poses Sequentially as Parameter Update Messages based on the current frame.
        mutex.lock();
        SerializePose(animData, charObj, sceneNodeList, msgBodyAnim, (int) animFrame);
        mutex.unlock();

        // Create ZMQ Message
        createNewMessage(_globalTimer->getLocalTimeStamp(), ZMQMessageHandler::MessageType::PARAMETERUPDATE, msgBodyAnim);

        // Sending LOCK message to the character (necessary for applying root animations)
        if (!locked) {
            createLockMessage(_globalTimer->getLocalTimeStamp(), charObj->sceneObjectID, true);
            int retunLockVal = sendSocket->send((void*)lockMessage->data(), lockMessage->size());
            locked = true;
        }

        // Sending new pose message
        int retunVal = sendSocket->send((void*)message->data(), message->size());


        if (animDataSize == 1) {    // IF   the animation data contains only one frame
            _paused = true;         // THEN stop the loop even if the loop check is marked
        }

        if (animFrame >= (animDataSize - 1) && loop) {    // IF   at the end of the animation AND LOOP is checked
            animFrame = 0;                                // THEN restart streaming the animation
        }
        else if (animFrame < animDataSize - 1) {          // IF   the animation has not been fully sent
            animFrame += deltaAnimFrame;                  // THEN increase the frame count by the given delta (animFrameRate / playbackFrameRate)
        }
        else {      
            mutex.lock();                                  // ELSE (the animation has been fully sent AND LOOP unchecked)
            streamAnimation = false;                       // THEN     stop the streaming loop
            mutex.unlock();
        }

        m_pauseMutex.unlock();

    }

    createLockMessage(_globalTimer->getLocalTimeStamp(), charObj->sceneObjectID, false);
    int retunUnlockVal = sendSocket->send((void*)lockMessage->data(), lockMessage->size());
    locked = false;


    //HOTFIX: Signal stream has finished by RPC, temporary adaption for compatibility with VPET
    //{
    //    qDebug() << "Sending PARAMETERUPDATE Stream Finished";
    //    int rpcOffsetID = 3 + 3; // TRS + HOTFIXPARAMS
    //    QByteArray msgStreamFinishedBody = CreateParameterUpdateBody<int>(targetSceneID, charObj->sceneObjectID, rpcOffsetID,
    //        ZMQMessageHandler::ParameterType::INT, 5);

    //    createNewMessage(_globalTimer->getLocalTimeStamp(), ZMQMessageHandler::MessageType::PARAMETERUPDATE, &msgStreamFinishedBody);
    //    int retunVal = sendSocket->send((void*)message->data(), message->size());
    //}
	


    // Set _working to false -> process cannot be aborted anymore
    mutex.lock();
    streamAnimation = false;
    mutex.unlock();
}

void AnimHostMessageSender::sendAnimationDataBlock()
{
    qDebug() << "Starting BLOCK AnimHost Message Sender";


    QByteArray* msgBodyAnim = new QByteArray();
   
    bool locked = false;

    m_pauseMutex.lock();

    mutex.lock();
    SerializeAnimation(animData, charObj, sceneNodeList, msgBodyAnim, 0);
    mutex.unlock();
    
    
    createNewMessage( _globalTimer->getLocalTimeStamp(), ZMQMessageHandler::MessageType::PARAMETERUPDATE, msgBodyAnim);

    // Sending LOCK message to the character (necessary for applying root animations)
    if (!locked) {
        createLockMessage(_globalTimer->getLocalTimeStamp(), charObj->sceneObjectID, true);
        int retunLockVal = sendSocket->send((void*)lockMessage->data(), lockMessage->size());
        locked = true;
    }

    // Sending new parameter update message
    int retunVal = sendSocket->send((void*)message->data(), message->size());

    QThread::msleep(1);

    m_pauseMutex.unlock();
        
    //UNLOCK CHARACTER
    createLockMessage(_globalTimer->getLocalTimeStamp(), charObj->sceneObjectID, false);
    int retunUnlockVal = sendSocket->send((void*)lockMessage->data(), lockMessage->size());
    locked = false;

    // Set _working to false -> process cannot be aborted anymore
    mutex.lock();
    sendBlock = false;
    mutex.unlock();

}



void AnimHostMessageSender::setAnimationAndSceneData(std::shared_ptr<Animation> ad, std::shared_ptr<CharacterObject> co, std::shared_ptr<SceneNodeObjectSequence> snl) {
    mutex.lock();
    animData = ad;
    charObj = co;
    sceneNodeList = snl;

    animDataSize = 0;
    for (Bone bone : animData->mBones) {
        int boneFrames = bone.mRotationKeys.size();
        if (boneFrames > animDataSize)
            animDataSize = boneFrames;
    }

    mutex.unlock();
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
    int charRootID = character->characterRootID;
    int nBones = character->skinnedMeshList.at(0).boneMapIDs.size();    // The number of Bones in the targeted character

    // Root TRS
    // Getting Bone Object Rotation Quaternion
    glm::quat boneQuat = animData->mBones.at(0).GetOrientation(frame);
    glm::vec3 bonePos = animData->mBones.at(0).GetPosition(frame);

    QByteArray msgRootPos = CreateParameterUpdateBody<glm::vec3>(targetSceneID, character->sceneObjectID, 0, // + 5,  // HOTFIX + 5 VPET DEMO
        ZMQMessageHandler::ParameterType::VECTOR3, bonePos);

    QByteArray msgRootQuat = CreateParameterUpdateBody<glm::quat>(targetSceneID, character->sceneObjectID, 1, // + 5, // HOTFIX + 5 VPET DEMO
        ZMQMessageHandler::ParameterType::QUATERNION, boneQuat);

    byteArray->append(msgRootPos);
    byteArray->append(msgRootQuat);

    for (ushort i = 0; i < nBones; i++) {
        // boneMapIDs contains the parameterID to boneID mapping
        //      parameterID (= i+3)                 id to be sent in the update message to the rendering application, the offset (+3) is necessary because the first 3 parameters are ALWAYS rootPos, rootRot, rootScl
        //      boneID      (= boneMapIDs.at(i))    id to be used to get BONE NAME given the list of SceneNodes in the received scene
        //      boneName                            name to be used to get BONE QUATERNION from the animation data
        // This WILL NOT WORK for RETARGETED animations

        // Getting boneName given the parameterID
        int boneID = character->skinnedMeshList.at(0).boneMapIDs.at(i);
        std::string boneName = sceneNodeList->mSceneNodeObjectSequence.at(boneID).objectName;


        if (boneName.compare("heel_02_R") == 0 || boneName.compare("heel_02_L") == 0) {
            qDebug() << "heel found";
            continue;
        }



        // boneName search (sequential...any ideas on how to make it faster?)
        int animDataBoneID = -1;
        for (int j = 0; j < animData->mBones.size(); j++) {
            if (boneName.compare(animData->mBones.at(j).mName) == 0) {
                animDataBoneID = j;
                break;
            }
        }
        
        // If animation data are applied to a bone, so the bone name has been found in animData at the position animDataBoneID
        // animDataBoneID is negative when the bone has no animation data associated to its name
        if (animDataBoneID >= 0) {
            //qDebug() << boneName << "not found in the Animation Data";

            // Getting Bone Object Rotation Quaternion
            glm::quat boneQuat = animData->mBones.at(animDataBoneID).GetOrientation(frame);
            glm::vec3 bonePos = animData->mBones.at(animDataBoneID).GetPosition(frame);


            QByteArray msgBoneQuat = CreateParameterUpdateBody<glm::quat>(targetSceneID, character->sceneObjectID, i + 3 + 3, // + 5, // HOTFIX + 5 VPET DEMO
                				ZMQMessageHandler::ParameterType::QUATERNION, boneQuat);
            
            QByteArray msgBonePos = CreateParameterUpdateBody<glm::vec3>(targetSceneID, character->sceneObjectID, i + nBones + 3 + 3, // + 5,  // HOTFIX + 5 VPET DEMO
                                                      ZMQMessageHandler::ParameterType::VECTOR3, bonePos);

            byteArray->append(msgBonePos);
            byteArray->append(msgBoneQuat);
        }
    }
}

void AnimHostMessageSender::SerializeAnimation(std::shared_ptr<Animation> animData, std::shared_ptr<CharacterObject> character,
    std::shared_ptr<SceneNodeObjectSequence> sceneNodeList, QByteArray* byteArray, int frame) {

    byteArray->clear();

    // Target Scene ID
    int targetSceneID = ZMQMessageHandler::getTargetSceneID();

    int nBones = character->skinnedMeshList.at(0).boneMapIDs.size();    // The number of Bones in the targeted character

	QString boneRotationMap = "";


    // Root TRS
    // Getting Bone Object Rotation Quaternion
    std::vector<std::pair<float, glm::vec3>> positionRootKeyPairs;
    std::vector<std::pair<float, glm::vec3>> tangentRootPositionKeyPairs;
    positionRootKeyPairs.reserve(animData->mBones.at(0).mPositonKeys.size());

    auto pKeysBegin = animData->mBones.at(0).mPositonKeys.begin();
    auto pKeysEnd = animData->mBones.at(0).mPositonKeys.end();

    std::transform(pKeysBegin, pKeysEnd, std::back_inserter(positionRootKeyPairs),
        [](const KeyPosition& key) {
            return std::make_pair(key.timeStamp / 60.f, key.position);
        });

    QByteArray msgRootPos = createAnimationParameterUpdateBody<glm::vec3>(targetSceneID, character->sceneObjectID, 0,
        ZMQMessageHandler::ParameterType::VECTOR3, ZMQMessageHandler::AnimationKeyType::STEP, positionRootKeyPairs, tangentRootPositionKeyPairs, frame);

    byteArray->append(msgRootPos);

    std::vector<std::pair<float, glm::quat>> rotationRootKeyPairs;
    std::vector<std::pair<float, glm::quat>> tangentRootRotationKeyPairs;
    rotationRootKeyPairs.reserve(animData->mBones.at(0).mRotationKeys.size());

    auto rKeysBegin = animData->mBones.at(0).mRotationKeys.begin();
    auto rKeysEnd = animData->mBones.at(0).mRotationKeys.end();

    std::transform(rKeysBegin, rKeysEnd, std::back_inserter(rotationRootKeyPairs),
        [](const KeyRotation& key) {
            return std::make_pair(key.timeStamp / 60.f, key.orientation);
        });

    QByteArray msgRootQuat = createAnimationParameterUpdateBody<glm::quat>(targetSceneID, character->sceneObjectID, 1,
        ZMQMessageHandler::ParameterType::QUATERNION, ZMQMessageHandler::AnimationKeyType::STEP, rotationRootKeyPairs, tangentRootRotationKeyPairs, frame);

    byteArray->append(msgRootQuat);




    for (int i = 0; i < nBones; i++)
    {

        int boneID = character->skinnedMeshList.at(0).boneMapIDs.at(i);
        std::string boneName = sceneNodeList->mSceneNodeObjectSequence.at(boneID).objectName;

		// HOTFIX Accomodate Survivor specific special case for heel_02_R and heel_02_L
		if (boneName.compare("heel_02_R") == 0 || boneName.compare("heel_02_L") == 0) {
			qDebug() << "heel found";
			continue;
		}

        int animDataBoneID = -1;
        for (int j = 0; j < animData->mBones.size(); j++) {
            if (boneName.compare(animData->mBones.at(j).mName) == 0) { // Bone names compare equal (case sensitive)
                animDataBoneID = j;
                break;
            }
        }
		if (animDataBoneID < 0) { // Continue if bone name not found in the animation data
            continue;
        }

        if (animData->mBones.at(animDataBoneID).mPositonKeys.size() != 0) {
            std::vector<std::pair<float, glm::vec3>> positionKeyPairs;
            std::vector<std::pair<float, glm::vec3>> tangentPositionKeyPairs;
            positionKeyPairs.reserve(animData->mBones.at(animDataBoneID).mPositonKeys.size());

            auto pKeysBegin = animData->mBones.at(animDataBoneID).mPositonKeys.begin();
            auto pKeysEnd = animData->mBones.at(animDataBoneID).mPositonKeys.end();

            std::transform(pKeysBegin, pKeysEnd, std::back_inserter(positionKeyPairs),
                [](const KeyPosition& key) {
                    return std::make_pair(key.timeStamp / 60.f , key.position);
                });

            QByteArray msgBonePos = createAnimationParameterUpdateBody<glm::vec3>(targetSceneID, character->sceneObjectID, i + nBones + 3 +3,
                ZMQMessageHandler::ParameterType::VECTOR3, ZMQMessageHandler::AnimationKeyType::STEP, positionKeyPairs, tangentPositionKeyPairs, frame);

            byteArray->append(msgBonePos);
        }
       

        std::vector<std::pair<float, glm::quat>> rotationKeyPairs;
        std::vector<std::pair<float, glm::quat>> tangentRotationKeyPairs;
        rotationKeyPairs.reserve(animData->mBones.at(animDataBoneID).mRotationKeys.size());

        auto rKeysBegin = animData->mBones.at(animDataBoneID).mRotationKeys.begin();
        auto rKeysEnd = animData->mBones.at(animDataBoneID).mRotationKeys.end();

        std::transform(rKeysBegin, rKeysEnd, std::back_inserter(rotationKeyPairs),
            [](const KeyRotation& key) {
                return std::make_pair(key.timeStamp / 60.f, key.orientation);
            });

        QByteArray msgBoneQuat = createAnimationParameterUpdateBody<glm::quat>(targetSceneID, character->sceneObjectID, i + 3 +3,
            ZMQMessageHandler::ParameterType::QUATERNION, ZMQMessageHandler::AnimationKeyType::STEP, rotationKeyPairs, tangentRotationKeyPairs, frame);

        byteArray->append(msgBoneQuat);
        
        boneRotationMap += QStringLiteral("{ % 1, \"%2\"},\n").arg((i + 3)).arg(QString::fromStdString(boneName));



    }

    qDebug() << boneRotationMap;

}




// Creating ZMQ Parameter Update Message Body from T value
template<typename T>
QByteArray AnimHostMessageSender::CreateParameterUpdateBody(byte sceneID, uint16_t objectID, uint16_t parameterID, ZMQMessageHandler::ParameterType parameterType, T payload) {

    byte targetSceneID;

    targetSceneID = getTargetSceneID();

    if (targetSceneID == -1)
        qWarning() << "Invalid target scene ID - 1.";

    uint32_t messageSize = 10 + getParameterDimension(parameterType);
    // Constructing new message
    QByteArray newMessage(0, Qt::Uninitialized);
    QDataStream msgStream(&newMessage, QIODevice::WriteOnly);
    msgStream.setByteOrder(QDataStream::LittleEndian);
    msgStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    msgStream << targetSceneID;     // Scene ID - 1 byte
    msgStream << objectID;          // Object ID - 2 bytes
    msgStream << parameterID;       // Parameter ID - 2 bytes
    msgStream << parameterType;		// Parameter Type - 1 byte
    msgStream << messageSize;		// Message Size - 4 bytes

    serializeValue<T>(msgStream, payload);

    return newMessage;
}

template<>
QByteArray AnimHostMessageSender::CreateParameterUpdateBody<std::string>(byte sceneID, uint16_t objectID, uint16_t parameterID, ZMQMessageHandler::ParameterType parameterType, std::string payload)
{
    byte targetSceneID;

    targetSceneID = getTargetSceneID();

    if (targetSceneID == -1)
        qWarning() << "Invalid target scene ID -1.";

    payload.shrink_to_fit();

    uint32_t messageSize = 10 + payload.length();
    // Constructing new message
    QByteArray newMessage(0, Qt::Uninitialized);
    QDataStream msgStream(&newMessage, QIODevice::WriteOnly);
    msgStream.setByteOrder(QDataStream::LittleEndian);
    msgStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    msgStream << targetSceneID;     // Scene ID - 1 byte
    msgStream << objectID;          // Object ID - 2 bytes
    msgStream << parameterID;       // Parameter ID - 2 bytes
    msgStream << parameterType;		// Parameter Type - 1 byte
    msgStream << messageSize;		// Message Size - 4 bytes

    serializeValue<std::string>(msgStream, payload);

    return newMessage;
}

template<typename T>
QByteArray AnimHostMessageSender::createAnimationParameterUpdateBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType, 
    ZMQMessageHandler::AnimationKeyType keytype,
    const std::vector<std::pair<float, T>>& Keys, 
    const std::vector < std::pair<float, T>>& tangentKeys, int frame) {

    bool useTangentKeys = true;

    byte targetSceneID;
   
    targetSceneID = getTargetSceneID();
    
    if (targetSceneID == -1)
        qDebug() << "Invalid target scene ID - 1.";

    //Check if the size of the keys and tangentKeys are the same
    if (Keys.size() != tangentKeys.size()) {
		//qDebug() << "WARNING::Keys and TangentKeys are not the same size.";
        useTangentKeys = false;
	}
  
    uint32_t payloadSize = Keys.size();
    uint32_t payloadSizeBytes = payloadSize * (1 + 3 * sizeof(float) + 3 *  getParameterDimension(parameterType)); // number of frames * (keytype + (key and tangent l/r time) + (key and tangent l/r data))
    uint32_t messageSize = 10 + getParameterDimension(parameterType) + sizeof(short) + payloadSizeBytes; // HEADER + PARAMETER_Data + NUMBER OF KEYS + Animation Payload
    // Constructing new message
    QByteArray newMessage(0, Qt::Uninitialized);
    QDataStream msgStream(&newMessage, QIODevice::WriteOnly);
    msgStream.setByteOrder(QDataStream::LittleEndian);
    msgStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    int debugSize = 0;

    debugSize = newMessage.size();

    msgStream << targetSceneID;                         // Scene ID - 1 byte
    msgStream << static_cast<uint16_t>(objectID);       // Object ID - 2 bytes
    msgStream << static_cast<uint16_t>(parameterID);    // Parameter ID - 2 bytes
    msgStream << parameterType;						    // Parameter Type - 1 byte

    msgStream << messageSize;						    // Message Size - 4 bytes

    debugSize = newMessage.size();

    serializeValue<T>(msgStream, Keys[frame].second);

    debugSize = newMessage.size();

    short numKeys = static_cast<uint16_t>(Keys.size());
    msgStream << numKeys;                               // NUMBER OF KEYS - 2 bytes

    // TODO:  add quaternions from payload
    for (int i = 0; i < Keys.size(); i++) {
        // to do: key type
        msgStream << keytype;

        msgStream << Keys[i].first; // KEY TIME

        if (useTangentKeys) {
            msgStream << tangentKeys[i].first;// TANGENT TIME
			msgStream << tangentKeys[i].first;// TANGENT TIME @TODO: add tangent data
        }
        else {
            float defaultTime = -1.0f;
            msgStream << defaultTime;

			msgStream << defaultTime;
        }

        // KEY DATA
        serializeValue<T>(msgStream, Keys[i].second);

        // TANGENT DATA
        if (useTangentKeys) {
            serializeValue<T>(msgStream, tangentKeys[i].second);
			serializeValue<T>(msgStream, tangentKeys[i].second); // @TODO: add tangent data
        }
        else {
            T defaultTangent = T();
            serializeValue<T>(msgStream, defaultTangent);

			serializeValue<T>(msgStream, defaultTangent); // @TODO: add tangent data
        }
            
    }

    return newMessage;
}
