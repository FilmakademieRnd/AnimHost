#ifndef ZMQMESSAGEHANDLER_H
#define ZMQMESSAGEHANDLER_H

#include <QObject>
#include <QMutex>
#include <QMultiMap>
#include <QElapsedTimer>
#include <nzmqt/nzmqt.hpp>
#include <zmq.hpp>
#include "C:\Qt\6.5.2\msvc2019_64\include\QtCore\qobject.h"

typedef unsigned char byte;


/**
* ABSTRACT CLASS acting as a base class for any ZMQ Message Handler
* - Senders, Receivers, Broadcasters, etc. -
* The concrete implementation is constraint by where and how the derived classes will be deployed
*/
class ZMQMessageHandler : public QObject {
    // IMPORTANT: uncomment the line below in the header of the concrete class
    //Q_OBJECT 
    public:
    ZMQMessageHandler() {};

    //request this process to start working
    virtual void requestStart() = 0; // TO BE IMPLEMENTED in concrete class

    //request this process to stop working
    virtual void requestStop() = 0; // TO BE IMPLEMENTED in concrete class

    void setDebugState(bool _debugState) {
        _debug = _debugState;
    }

    void setMessage(zmq::message_t* msg) {
        qDebug() << "Setting message of size " << msg->size();
        message = msg;
        qDebug() << "Setting message of size " << message->size();

    }

    enum MessageType {
        PARAMETERUPDATE, LOCK, // node
        SYNC, PING, RESENDUPDATE, // sync
        UNDOREDOADD, RESETOBJECT, // undo redo
        DATAHUB, // DataHub
        EMPTY = 255
    };

    protected:

    //id displayed as clientID for messages redistributed through syncServer
    byte targetHostID = 0;

    //if true process is stopped
    bool _stop = true;

    //shall debug messages be printed
    bool _debug = false;

    //if true process is running
    bool _working = false;

    //protect access to _stop
    QMutex mutex;

    //zeroMQ context
    zmq::context_t* context = nullptr;

    //zeroMQ message exposed to be populated before being passed along the data pipeline
    zmq::message_t* message = nullptr;;

    //syncMessage: includes targetHostID, timestamp? and size of the message (as defined by MessageType enum)
    byte syncMessage[3] = { targetHostID,0,MessageType::EMPTY };

    //server IP
    QString ipAddress;

    //map of last states
    QMap<QByteArray, QByteArray> objectStateMap;

    //map of ping timings
    QMap<byte, unsigned int> pingMap;

    //map of last states
    QMultiMap<byte, QByteArray> lockMap;

    //the local elapsed time in seconds since object has been created.
    unsigned int m_time = 0;

    static const unsigned int m_pingTimeout = 4;

    inline const short CharToShort(const char* buf) const {
        short val;
        std::memcpy(&val, buf, 2);
        return val;
    }

    signals:
    //signal emitted when process requests to work
    virtual void startRequested() = 0;

    //signal emitted when process is finished
    virtual void stopped() = 0;

    public slots:
    //execute operations
    virtual void run() = 0;

    private slots:
    //create a new sync message
    virtual void createSyncMessage(int time) = 0;
};

//class ZMQMessageHandler :
//  public QObject {};

#endif // ZMQMESSAGEHANDLER_H
