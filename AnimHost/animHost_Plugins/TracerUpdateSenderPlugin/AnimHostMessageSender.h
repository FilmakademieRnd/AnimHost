#ifndef ANIMHOSTMESSAGESENDER_H
#define ANIMHOSTMESSAGESENDER_H

#include <QObject>
#include <QMutex>
#include <QMultiMap>
#include <QElapsedTimer>
#include <nzmqt/nzmqt.hpp>
#include "ZMQMessageHandler.h"

class AnimHostMessageSender : public ZMQMessageHandler {
    Q_OBJECT
    public:
    AnimHostMessageSender() {}
    AnimHostMessageSender(QString m_ipAddress) {
        ipAddress = m_ipAddress;
    }
    AnimHostMessageSender(QString m_ipAddress, bool m_debugState, zmq::context_t * m_context) {
        ipAddress = m_ipAddress;
        _debug = m_debugState;
        context = m_context;
        _stop = false;
        _working = false;
    }
    ~AnimHostMessageSender() {
        sendSocket->close();
    }

    //request this process to start working
    void requestStart() override;

    //request this process to stop working
    void requestStop() override;

    private:

    // ZeroMQ Socket used to send animation data
    zmq::socket_t* sendSocket = nullptr;

    //id displayed as clientID for messages redistributed through syncServer
    //byte targetHostID;

    //syncMessage
    //byte syncMessage[3] = { targetHostID,0,MessageType::EMPTY };

    //server IP
    //QString IPadress;

    //map of last states
    //QMap<QByteArray, QByteArray> objectStateMap;

    //map of ping timings
    //QMap<byte, unsigned int> pingMap;

    //map of last states
    //QMultiMap<byte, QByteArray> lockMap;

    //the local elapsed time in seconds since object has been created.
    //unsigned int m_time = 0;

    //static const unsigned int m_pingTimeout = 4;


    signals:
    //signal emitted when process requests to work
    void startRequested();

    //signal emitted when process is finished
    void stopped();

    public slots:
    //execute operations
    void run() override;

    private slots:
    //create a new sync message
    void createSyncMessage(int time);
};

#endif // ANIMHOSTMESSAGESENDER_H
