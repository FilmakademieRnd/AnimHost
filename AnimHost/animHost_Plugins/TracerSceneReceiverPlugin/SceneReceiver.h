
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

    void sendRequest(QString newIPAddress, QString request) {
        //! (Re-)connect to newIPAddress
        /*if (receiveSocket->connected())
            receiveSocket->disconnect(QString("tcp://" + ipAddress + ":5555").toLatin1().data());
        setIPAddress(newIPAddress);
        receiveSocket->connect(QString("tcp://" + ipAddress + ":5555").toLatin1().data());*/

        receiveSocket->connect(QString("tcp://" + ipAddress + ":5555").toLatin1().data());

        //! requesting the character list in the TRACER client scene
        requestMsg.rebuild(request.toLatin1().data(), request.size());
        qDebug() << "Requesting Scene" << request.toLatin1().data() << "(size " << request.size() << ")";
        receiveSocket->send(requestMsg);

        receiveSocket->recv(&replyMsg);
        qDebug() << "Reply received!";

        // replyMsg will contain a series of bytes representing all the Character Packages in the scene
        if (replyMsg.size() > 0) {
            QByteArray* msgArray = new QByteArray((char*) replyMsg.data(), static_cast<int>(replyMsg.size())); // Convert message into explicit byte array
            passCharacterByteArray(msgArray); // Pass byte array to TracerSceneReceiverPlugin
        }
    }

    private:
    zmq::socket_t* receiveSocket = nullptr;
    TracerSceneReceiverPlugin* TSRPlugin = nullptr;
    zmq::message_t requestMsg {};
    zmq::message_t replyMsg {};

    public Q_SLOTS:

    void run() {
        //// open socket
        ////receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req);
        //receiveSocket->connect(QString("tcp://" + ipAddress + ":5555").toLatin1().data());
        //receiveSocket->setsockopt(ZMQ_REQ, "request scene", 0);

        ////! Listen for reply message
        //receiveSocket->recv(replyMsg);
        //qDebug() << "Reply received!";

        //// replyMsg will contain a series of bytes representing all the Character Packages in the scene
        //if (replyMsg->size() > 0) {
        //    QByteArray* msgArray = new QByteArray((char*) replyMsg->data(), static_cast<int>(replyMsg->size())); // Convert message into explicit byte array
        //    passCharacterByteArray(msgArray); // Pass byte array to TracerSceneReceiverPlugin
        //}
    }

    Q_SIGNALS:
    void stopped();
    void passCharacterByteArray(QByteArray* msgArray);
};
#endif // SCENERECEIVER_H