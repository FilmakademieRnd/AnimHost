
#ifndef TICKRECEIVER_H
#define TICKRECEIVER_H

#include "ZMQMessageHandler.h"
#include "TracerUpdateSenderPlugin.h"

//#include <QObject>
#include <QMutex>
#include <QThread>
#include <nzmqt/nzmqt.hpp>

class TRACERUPDATESENDERPLUGINSHARED_EXPORT TickReceiver : public ZMQMessageHandler {

	Q_OBJECT

	public:
	TickReceiver() {}
    TickReceiver(TracerUpdateSenderPlugin* m_TUSPlugin, QString m_ipAddress, bool m_debugState, zmq::context_t* m_context) {
        TUSPlugin = m_TUSPlugin;
        ipAddress = m_ipAddress;
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;
    }

    TickReceiver(QString m_ipAddress, bool m_debugState, zmq::context_t* m_context) {
        ipAddress = m_ipAddress;
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;
    }

	~TickReceiver() {
        receiveSocket->close();
    }

    void requestStart() override {
        mutex.lock();
        _working = true;
        _stop = false;
        _paused = false;
        qDebug() << "AnimHost Tick Receiver requested to start";// in Thread "<<thread()->currentThreadId();
        mutex.unlock();
    }

    void requestStop() override {
        mutex.lock();
        if (_working) {
            _stop = true;
            _paused = false;
            //_working = false;
            qDebug() << "AnimHost Tick Receiver stopping";// in Thread "<<thread()->currentThreadId();
        }
        mutex.unlock();
    }

    private:
    zmq::socket_t* receiveSocket = nullptr;
    TracerUpdateSenderPlugin* TUSPlugin = nullptr;

	public Q_SLOTS:

    void run() {
        // open socket
        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::sub);
        receiveSocket->connect(QString("tcp://" + ipAddress + ":5556").toLatin1().data());
        receiveSocket->setsockopt(ZMQ_SUBSCRIBE, "client", 0);

        zmq::pollitem_t pollItem = { static_cast<void*>(*receiveSocket), 0, ZMQ_POLLIN, 0 };

        zmq::message_t recvMsg;

        while (_working) {
            qDebug() << "Waiting for SYNC message...";
            // listen for tick
            zmq::poll(&pollItem, 1, -1);
            qDebug() << "Processing message...";

            if (pollItem.revents & ZMQ_POLLIN) {
                //try to receive a zeroMQ message
                receiveSocket->recv(&recvMsg);
            }

            if (recvMsg.size() > 0) {
                QByteArray msgArray = QByteArray((char*) recvMsg.data(), static_cast<int>(recvMsg.size())); // Convert message into explicit byte array
                const MessageType msgType = static_cast<MessageType>(msgArray[2]);                          // Extract message type from byte array (always third byte)

                //if tick received with time tickTime
                if (msgType == MessageType::SYNC) { // Should we check also against Client ID?
                    const int syncTime = static_cast<int>(msgArray[1]); // Extract sync time from message (always second byte)
                    qDebug() << "SYNC message received with time " << syncTime;
                    mutex.lock();
                    tick(syncTime);
                    mutex.unlock();
                }
            }
        }
    }

    Q_SIGNALS:
    void tick(int syncTime);
    void stopped();
};
#endif // TICKRECEIVER_H
