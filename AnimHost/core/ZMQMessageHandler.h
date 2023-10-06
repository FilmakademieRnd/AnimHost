#ifndef ZMQMESSAGEHANDLER_H
#define ZMQMESSAGEHANDLER_H

#include <QtCore/QObject>
#include "commondatatypes.h"
#include <QMutex>
#include <QObject>
#include <QMultiMap>
#include <QElapsedTimer>
#include <QWaitCondition>
#include <any>
#include <nzmqt/nzmqt.hpp>
#include <zmq.hpp>

typedef unsigned char byte;


/**
* ABSTRACT CLASS acting as a base class for any ZMQ Message Handler
* - Senders, Receivers, Broadcasters, etc. -
* The concrete implementation is constraint by where and how the derived classes will be deployed
*/
class ANIMHOSTCORESHARED_EXPORT ZMQMessageHandler : public QObject {
    
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

    void resume();

    void pause() {
        mutex.lock();
        _paused = true;
        mutex.unlock();
    }

    enum MessageType {
        PARAMETERUPDATE, LOCK, // node
        SYNC, PING, RESENDUPDATE, // sync
        UNDOREDOADD, RESETOBJECT, // undo redo
        DATAHUB, // DataHub
        EMPTY = 255
    };

    enum ParameterType : byte {
        NONE, ACTION, BOOL,                     // Generic
        INT, FLOAT,                             // Scalar
        VECTOR2, VECTOR3, VECTOR4, QUATERNION,  // Vectors
        COLOR, STRING, LIST,                    // Other Data Structures
        UNKNOWN = 100
    };

    static constexpr byte getParameterDimension(ParameterType parameterType) {
        return parameterDimension[parameterType];
    }

    //enum ParameterDim : byte {
    //    NONE = 0, ACTION = 1, BOOL = 2,                             // Generic
    //    INT = 2, FLOAT = 4,                                         // Scalar
    //    VECTOR2 = 8, VECTOR3 = 12, VECTOR4 = 16, QUATERNION = 16,   // Vectors
    //    COLOR = 3, STRING = 100, LIST = 100,                        // Other Data Structures
    //    UNKNOWN = 100
    //};

    void setTargetHostID(byte _targetHostID) {
        targetHostID = _targetHostID;
    }

    byte getTargetHostID() {
        return targetHostID;
    }

    // Converting elements in data into bytes
    void Serialize(byte* data, ParameterType type);

    zmq::message_t* createMessage(byte targetHostID, byte time, ZMQMessageHandler::MessageType messageType,
                             byte SceneID, byte objectID, byte ParameterID, ZMQMessageHandler::ParameterType paramType,
                             byte* payload, size_t payloadSize);

    protected:

    //id displayed as clientID for messages redistributed through syncServer
    byte targetHostID = 0;

    //if true process is stopped
    bool _stop = true;

    //shall debug messages be printed
    bool _debug = false;

    //if true process is running
    bool _working = false;

    //if true process is running but paused
    bool _paused = false;

    //protect access to _stop
    QMutex mutex;

    //handles pause/resume signals
    QWaitCondition waitCondition;

    //zeroMQ context
    zmq::context_t* context = nullptr;

    //zeroMQ message exposed to be populated before being passed along the data pipeline
    zmq::message_t message;

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

    // Storing parameter dimensions (NONE, ACTION, BOOL, INT, FLOAT, VECTOR2, VECTOR3, VECTOR4, QUATERNION, COLOR-RGBA, STRING, LIST, UNKNOWN respectively)
    static constexpr byte parameterDimension[13] = {
        0, 1, sizeof(bool),
        sizeof(int), sizeof(float),
        sizeof(float)*2, sizeof(float)*3, sizeof(float)*4, sizeof(float)*4,
        sizeof(float)*4, 100, 100, 100};
    
    const short CharToShort(const char* buf) const {
        short val;
        std::memcpy(&val, buf, 2);
        return val;
    }

    signals :
    //signal emitted when process is finished
    void stopped();

    public slots:
    //execute operations
    virtual void run() {};

    protected slots:
    //create a new sync message
    void createSyncMessage(int time) {
        syncMessage[0] = targetHostID;
        syncMessage[1] = time;
        syncMessage[2] = MessageType::SYNC;

        // increase local time for controlling client timeouts
        m_time++;
    }
};

//class ZMQMessageHandler :
//  public QObject {};

#endif // ZMQMESSAGEHANDLER_H
