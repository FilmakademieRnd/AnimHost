
#ifndef SCENERECEIVER_H
#define SCENERECEIVER_H

#include "ZMQMessageHandler.h"
#include "TracerSceneReceiverPlugin.h"

//#include <QObject>
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
    }

    SceneReceiver(QString m_ipAddress, bool m_debugState, zmq::context_t* m_context) {
        ipAddress = m_ipAddress;
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;
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

    private:
    zmq::socket_t* receiveSocket = nullptr;
    TracerSceneReceiverPlugin* TSRPlugin = nullptr;

    public Q_SLOTS:

    void run() {
        // open socket
        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req);
        receiveSocket->connect(QString("tcp://" + ipAddress + ":5554").toLatin1().data()); //TODO: check correct port!!!
        receiveSocket->setsockopt(ZMQ_DEALER, "request scene", 0);

        zmq::pollitem_t pollItem = { static_cast<void*>(*receiveSocket), 0, ZMQ_POLLIN, 0 };

        QString request = "Scene request";
        zmq::message_t* requestMsg = new zmq::message_t(request.data(), request.size());
        zmq::message_t* replyMsg = new zmq::message_t();

        receiveSocket->send(*requestMsg);
        qDebug() << "Waiting for Scene Reply message...";
        // listen for reply
        receiveSocket->recv(replyMsg);
        qDebug() << "Processing message...";

        if (replyMsg->size() > 0) {
            QByteArray* msgArray = new QByteArray((char*) replyMsg->data(), static_cast<int>(replyMsg->size())); // Convert message into explicit byte array
            const MessageType msgType = static_cast<MessageType>(msgArray->at(2));                              // Extract message type from byte array (always third byte)

            // if the message type matches (should we create an ad-hoc message type for this?)
            if (msgType == MessageType::SYNC) { // Should we check also against Client ID?
                // consume scene description message (removing header)
                msgArray->remove(0, 3);
                // return the unstructured data to TracerSceneReceiverPlugin
                sceneReceived(msgArray);
            }
        }    
    }

    Q_SIGNALS:
    void sceneReceived(QByteArray* sceneDescription);
    void stopped();
};
#endif // SCENERECEIVER_H