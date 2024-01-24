#ifndef ANIMHOSTMESSAGESENDER_H
#define ANIMHOSTMESSAGESENDER_H

#include "ZMQMessageHandler.h"
#include "TracerUpdateSenderPlugin.h"

//#include <QObject>
#include <QMutex>
#include <QMultiMap>
#include <QElapsedTimer>
#include <nzmqt/nzmqt.hpp>
#include <zmq.h>


class TRACERUPDATESENDERPLUGINSHARED_EXPORT AnimHostMessageSender : public ZMQMessageHandler {
    
    Q_OBJECT

    public:
    AnimHostMessageSender() {}

    AnimHostMessageSender(bool m_debugState, zmq::context_t * m_context) {
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;
    }
    ~AnimHostMessageSender() {
        //sendSocket->close();
    }

    //request this process to start working
    void requestStart() override;

    //request this process to stop working
    void requestStop() override;

    void setAnimationAndSceneData(std::shared_ptr<Animation> ad, std::shared_ptr<CharacterObject> co, std::shared_ptr<SceneNodeObjectSequence> snl);

    QByteArray createMessageBody(byte SceneID, int objectID, int ParameterID, ZMQMessageHandler::ParameterType paramType,
                                 bool payload);
    QByteArray createMessageBody(byte SceneID, int objectID, int ParameterID, ZMQMessageHandler::ParameterType paramType,
                                 std::int32_t payload);
    QByteArray createMessageBody(byte SceneID, int objectID, int ParameterID, ZMQMessageHandler::ParameterType paramType,
                                 float payload);
    QByteArray createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
                                 std::string payload);
    QByteArray createMessageBody(byte SceneID, int objectID, int ParameterID, ZMQMessageHandler::ParameterType paramType,
                                 std::vector<float> payload);

    void SerializeVector(byte* dest, std::vector<float> _vector, ZMQMessageHandler::ParameterType type);

    // Serializes in the form of a byte array an animation frame represented by a list of quaternions (Animation data type)
    void SerializePose(std::shared_ptr<Animation> animData, std::shared_ptr<CharacterObject> character,
                       std::shared_ptr<SceneNodeObjectSequence> sceneNodeList, QByteArray* byteArray, int frame = 0);

    //! Does the sent animation loop?
    bool loop = false;

    std::shared_ptr<Animation> animData = nullptr;
    std::shared_ptr<CharacterObject> charObj = nullptr;
    std::shared_ptr<SceneNodeObjectSequence> sceneNodeList = nullptr;

    private:
    // ZeroMQ Socket used to send animation data
    zmq::socket_t* sendSocket = nullptr;

    //syncMessage
    //byte syncMessage[3] = { targetHostID,0,MessageType::EMPTY };

    signals:
    //signal emitted when process requests to work
    //void startRequested();

    //signal emitted when process is finished
    void stopped();

    public Q_SLOTS:
    //execute operations
    void run();
};

#endif // ANIMHOSTMESSAGESENDER_H
