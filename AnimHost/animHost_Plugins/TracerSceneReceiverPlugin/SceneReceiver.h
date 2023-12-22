
#ifndef SCENERECEIVER_H
#define SCENERECEIVER_H

#include "ZMQMessageHandler.h"
#include "TracerSceneReceiverPlugin.h"

#include <QMutex>
#include <QThread>
#include <nzmqt/nzmqt.hpp>

class TRACERSCENERECEIVERPLUGINSHARED_EXPORT SceneReceiver : public ZMQMessageHandler {

    Q_OBJECT

    public:
    SceneReceiver() {}
    SceneReceiver(TracerSceneReceiverPlugin* m_TSRPlugin, QString m_ipAddress, bool m_debugState, zmq::context_t* m_context) {
        TSRPlugin = m_TSRPlugin;
        ipAddress = m_ipAddress;
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;

        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req);

        // THIS TRIGGERS NULLPOINTER-RELATED EXCEPTION
        // open socket
        /*receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req);
        receiveSocket->connect(QString("tcp://" + ipAddress + ":5555").toLatin1().data());
        receiveSocket->setsockopt(ZMQ_REQ, "request scene", 0);*/
    }

    SceneReceiver(QString m_ipAddress, bool m_debugState, zmq::context_t* m_context) {
        ipAddress = m_ipAddress;
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;

        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req);

        // THIS TRIGGERS NULLPOINTER-RELATED EXCEPTION
        // open socket
        /*receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req);
        receiveSocket->connect(QString("tcp://" + ipAddress + ":5555").toLatin1().data());
        receiveSocket->setsockopt(ZMQ_REQ, "request scene", 0);*/
    };

    ~SceneReceiver() {
        receiveSocket->close();
    };

    void requestStart() override {
        mutex.lock();
        _working = true;
        _stop = false;
        _paused = false;
        qDebug() << "AnimHost Scene Receiver requested to start";// in Thread "<<thread()->currentThreadId();
        mutex.unlock();
    };

    void requestStop() override {
        mutex.lock();
        if (_working) {
            _stop = true;
            _paused = false;
            //_working = false;
            qDebug() << "AnimHost Scene Receiver stopping";// in Thread "<<thread()->currentThreadId();
        }
        mutex.unlock();
    };

    //! requesting the Character Package list in the TRACER client scene
    void requestSceneCharacterData() {
        requestMsg.rebuild("characters", 10);
        qDebug() << "Requesting character packages";
        receiveSocket->send(requestMsg);

        receiveSocket->recv(&replyCharMsg);
        qDebug() << "Reply received!";

        QByteArray* charPkgArray = new QByteArray();
        // replyCharMsg will contain a series of bytes representing all the Scene Nodes in the scene
        if (replyCharMsg.size() > 0)
            charPkgArray->append((char*)replyCharMsg.data(), replyCharMsg.size());
            //memcpy(charPkgArray->data(), replyCharMsg.data(), replyCharMsg.size());
            //charPkgArray->push_back((char*) replyCharMsg.data()); // Convert message into explicit byte array

        passCharacterByteArray(charPkgArray); // Pass byte array to TracerSceneReceiverPlugin to be parsed as chracter packages
    }

    //! requesting the SceneNode list in the TRACER client scene
    void requestSceneNodeData() {    
        requestMsg.rebuild("nodes", 5);
        qDebug() << "Requesting nodes";
        receiveSocket->send(requestMsg);

        receiveSocket->recv(&replyNodeMsg);
        qDebug() << "Reply received!";

        QByteArray* nodesArray = new QByteArray();
        // replyNodeMsg will contain a series of bytes representing all the Scene Nodes in the scene
        if (replyNodeMsg.size() > 0)
            nodesArray->append((char*) replyNodeMsg.data(), replyNodeMsg.size());

        passSceneNodeByteArray(nodesArray); // Pass byte array to TracerSceneReceiverPlugin to be parsed as scene nodes
    }

    // TODO: requesting the header of the TRACER client
    //void requestHeaderData() {}

    void connectSocket(QString newIPAddress) {
        //! (Re-)connect to newIPAddress
        /*if (receiveSocket->connected())
            receiveSocket->disconnect(QString("tcp://" + ipAddress + ":5555").toLatin1().data()); //disconnect throws unexpected exception*/

        setIPAddress(newIPAddress);
        receiveSocket->connect(QString("tcp://" + ipAddress + ":5555").toLatin1().data());
    }

    private:
    zmq::socket_t* receiveSocket = nullptr;
    TracerSceneReceiverPlugin* TSRPlugin = nullptr;
    zmq::message_t requestMsg {};
    zmq::message_t replyNodeMsg {};
    zmq::message_t replyCharMsg {};

    public Q_SLOTS:

    void run() {}

    Q_SIGNALS:
    void stopped();
    void passCharacterByteArray(QByteArray* characterMsgArray);
    void passSceneNodeByteArray(QByteArray* sceneNodeMsgArray);
    //void passHeaderByteData(QByteArray* headerMsgArray);
};
#endif // SCENERECEIVER_H