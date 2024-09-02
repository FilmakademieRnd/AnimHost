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
#include <QDataStream>
#include <iostream>

void AnimHostMessageSender::requestStart() {
    mutex.lock();
    _working = true;
    _stop = false;
    _paused = false;
    //ZMQMessageHandler::localTick->start();
    qDebug() << "AnimHost Message Sender requested to start";// in Thread "<<thread()->currentThreadId();

    sendSocket = new zmq::socket_t(*context, zmq::socket_type::pub); // publisher socket
    mutex.unlock();
}

void AnimHostMessageSender::requestStop() {
    mutex.lock();
    if (_working) {
        _stop = true;
        _paused = false;
        _working = false;
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

    while (!_stop) { //loop until stop is requested (by calling requestStop(); on deletion of sender node in compute graph)

        if (streamAnimation) {   
            streamAnimationData(); // on completion of streaming the animation, the streamAnimation flag is set to false
        }
        else if (!streamAnimation && sendBlock) {
            sendAnimationDataBlock(); // on completion of sending the animation block, the sendBlock flag is set to false
        }

    };

    qDebug() << "AnimHost Message Sender process to be stopped";

    emit stopped();
}


void AnimHostMessageSender::streamAnimationData()
{
    qDebug() << "Starting STREAM AnimHost Message Sender";// in Thread " << thread()->currentThreadId();

    //zmq::message_t* tempMsg = new zmq::message_t();

    // Allows up- and down-sampling of the animation in order to keep the perceived speed the same even though the playback framerate is not the same
    // w.r.t. the framerate, for which the animation was designed
    deltaAnimFrame = (float) 1; //_globalTimer->getAnimFrameRate() / _globalTimer->getPlaybackFrameRate();

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
        //if (!sendFrameWaitCondition->wait(&m_pauseMutex, 100)) {

        //    // If TRACER Tick is not received, stop the process.
        //    // Currently occurs when UI Drawing is too slow & 
        //    // message sender thread is destroyed, as aresult of deleting the sender node
        //    // in the compute graph.
       
        //    qDebug() << "Timeout in AnimHost Message Sender";
        //    m_pauseMutex.unlock();
        //    break;
        //}

        _globalTimer->waitOnTick();
  

        // Send Poses Sequentially as Parameter Update Messages based on the current frame.
        mutex.lock();
        SerializePose(animData, charObj, sceneNodeList, msgBodyAnim, (int) animFrame);
        mutex.unlock();

        // Create ZMQ Message
        // -> implemented in parent class because the message header is the same for all messages emitted from same client (not unique to parameter update messages)
        createNewMessage(_globalTimer->getLocalTimeStamp(), ZMQMessageHandler::MessageType::PARAMETERUPDATE, msgBodyAnim);

        // Check message data
        /*std::string debugOut;
        char* debugDataArray = message->data();
        for (int i = 0; i < message->size() / sizeof(char); i++) {
            debugOut = debugOut + std::to_string(debugDataArray[i]) + " ";
        }*/

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

    // Set _working to false -> process cannot be aborted anymore
    mutex.lock();
    streamAnimation = false;
    mutex.unlock();
}

void AnimHostMessageSender::sendAnimationDataBlock()
{
    qDebug() << "Starting BLOCK AnimHost Message Sender";// in Thread " << thread()->currentThreadId();

    //zmq::message_t* tempMsg = new zmq::message_t();

    // Allows up- and down-sampling of the animation in order to keep the perceived speed the same even though the playback framerate is not the same
    // w.r.t. the framerate, for which the animation was designed
    QByteArray* msgBodyAnim = new QByteArray();

    // The length of the animation is defined by the bones that has the more frames
   


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

    // Create messages for sending out the Character Root TRS and appending them to the byte array that is going to be sent to TRACER applications
    //QByteArray msgRootPos = createMessageBody(targetSceneID, character->sceneObjectID, 0, ZMQMessageHandler::ParameterType::VECTOR3,    rootPosVector);
    //byteArray->append(msgRootPos);
    //QByteArray msgRootRot = createMessageBody(targetSceneID, character->sceneObjectID, 1, ZMQMessageHandler::ParameterType::QUATERNION, rootRotVector);
    //byteArray->append(msgRootRot);
    //QByteArray msgRootScl = createMessageBody(targetSceneID, character->sceneObjectID, 2, ZMQMessageHandler::ParameterType::VECTOR3,    rootSclVector);
    //byteArray->append(msgRootScl);
    
    int charRootID = character->characterRootID;
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
        
        // If animation data are applied to a bone, so the bone name has been found in animData at the position animDataBoneID
        // animDataBoneID is negative when the bone has no animation data associated to its name
        if (animDataBoneID >= 0) {
            //qDebug() << boneName << "not found in the Animation Data";

            // Getting Bone Object Rotation Quaternion
            glm::quat boneQuat = animData->mBones.at(animDataBoneID).GetOrientation(frame);
            glm::vec3 bonePos = animData->mBones.at(animDataBoneID).GetPosition(frame);


            QByteArray msgBoneQuat = CreateParameterUpdateBody<glm::quat>(targetSceneID, character->sceneObjectID, i + 3,
                				ZMQMessageHandler::ParameterType::QUATERNION, boneQuat);
            
            QByteArray msgBonePos = CreateParameterUpdateBody<glm::vec3>(targetSceneID, character->sceneObjectID, i + nBones + 3,
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

    for (int i = 0; i < nBones; i++)
    {

        int boneID = character->skinnedMeshList.at(0).boneMapIDs.at(i);
        std::string boneName = sceneNodeList->mSceneNodeObjectSequence.at(boneID).objectName;


        int animDataBoneID = -1;
        for (int j = 0; j < animData->mBones.size(); j++) {
            if (boneName.compare(animData->mBones.at(j).mName) == 0) { //bone names compare equal (case sensitive)
                animDataBoneID = j;
                break;
            }
        }
        if (animDataBoneID < 0) {
            //qDebug() << boneName << "not found in the Animation Data";
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
                    return std::make_pair(key.timeStamp, key.position);
                });

            QByteArray msgBonePos = createAnimationParameterUpdateBody<glm::vec3>(targetSceneID, character->sceneObjectID, i + nBones + 3,
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
                return std::make_pair(key.timeStamp, key.orientation);
            });

        QByteArray msgBoneQuat = createAnimationParameterUpdateBody<glm::quat>(targetSceneID, character->sceneObjectID, i + 3,
            ZMQMessageHandler::ParameterType::QUATERNION, ZMQMessageHandler::AnimationKeyType::STEP, rotationKeyPairs, tangentRotationKeyPairs, frame);

        byteArray->append(msgBoneQuat);

    }

}




// Creating ZMQ Parameter Update Message Body from T value
template<typename T>
QByteArray AnimHostMessageSender::CreateParameterUpdateBody(byte sceneID, uint16_t objectID, uint16_t parameterID, ZMQMessageHandler::ParameterType parameterType, T payload) {

    byte targetSceneID;

    targetSceneID = getTargetSceneID();

    if (targetSceneID == -1)
        qDebug() << "WARNING::Invalid target scene ID -1.";

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

    //msgStream.writeRawData(reinterpret_cast<const char*>(&payload), getParameterDimension(parameterType));
    serializeValue<T>(msgStream, payload);

    return newMessage;
}

template<>
QByteArray AnimHostMessageSender::CreateParameterUpdateBody<std::string>(byte sceneID, uint16_t objectID, uint16_t parameterID, ZMQMessageHandler::ParameterType parameterType, std::string payload)
{
    byte targetSceneID;

    targetSceneID = getTargetSceneID();

    if (targetSceneID == -1)
        qDebug() << "WARNING::Invalid target scene ID -1.";

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

    //msgStream.writeRawData(reinterpret_cast<const char*>(&payload), getParameterDimension(parameterType));
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
        qDebug() << "WARNING::Invalid target scene ID -1.";

    //Check if the size of the keys and tangentKeys are the same
    if (Keys.size() != tangentKeys.size()) {
		//qDebug() << "WARNING::Keys and TangentKeys are not the same size.";
        useTangentKeys = false;
	}
  
    //qDebug() << "Parameter Size:" << getParameterDimension(parameterType);
    uint32_t payloadSize = Keys.size();
    uint32_t payloadSizeBytes = payloadSize * (1 + 2 * sizeof(float) + 2 *  getParameterDimension(parameterType)); // number of frames * (keytype + (key and tangent time) + (key and tangent data))
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

    //qDebug() << "Calc Message size: " << messageSize;
    msgStream << messageSize;						   // Message Size - 4 bytes

    debugSize = newMessage.size();

    //msgStream.writeRawData(reinterpret_cast<const char*>(&Keys[frame].second), getParameterDimension(parameterType)); 

    serializeValue<T>(msgStream, Keys[frame].second);

    debugSize = newMessage.size();

    short numKeys = static_cast<uint16_t>(Keys.size());
    msgStream << numKeys; // NUMBER OF KEYS - 2 bytes

    // TODO:  add quaternions from payload
    for (int i = 0; i < Keys.size(); i++) {
        // to do: key type
        msgStream << keytype;

        msgStream << Keys[i].first; // KEY TIME

        if (useTangentKeys) {
            msgStream << tangentKeys[i].first;// TANGENT TIME
        }
        else {
            float defaultTime = -1.0f;
            msgStream << defaultTime;
        }
            

        // KEY DATA
        serializeValue<T>(msgStream, Keys[i].second);

        // TANGENT DATA
        if (useTangentKeys) {
            serializeValue<T>(msgStream, tangentKeys[i].second);
        }
        else {
            T defaultTangent = T();
            serializeValue<T>(msgStream, defaultTangent);
        }
            
    }

    //qDebug() << "True Message size: " << newMessage.size();
    return newMessage;
}
