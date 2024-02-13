
//!
//! \file "ZMQMessageHandler.h"
//! \brief Class used to build and send udpate messages to TRACER clients     
//! \author Francesco Andreussi
//! \version 0.5
//! \date 26.01.2024
//!
/*!
 * ###This is a abstract class acting as a base class for any ZMQ Message Handler. It has to be implemented by a derived classes.
 * The class provides base methods and fields that can be useful to multiple classes that will have to interact with TRACER (DataHub and other clients alike),
 * for instance by sending and receiving 0MQ messages in a format specified by TRACER.
 */

#ifndef ZMQMESSAGEHANDLER_H
#define ZMQMESSAGEHANDLER_H

#include <QtCore/QObject>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QHostAddress>
#include "commondatatypes.h"
#include <QMutex>
#include <QObject>
#include <QMultiMap>
#include <QElapsedTimer>
#include <QWaitCondition>
#include <QValidator>

#include <any>
#include <stdio.h>
#include <string>
#include <vector>


#include <nzmqt/nzmqt.hpp>
#include <zmq.hpp>

typedef unsigned char byte; //!< \typedef unsigned char byte

class ANIMHOSTCORESHARED_EXPORT ZMQMessageHandler : public QObject {
    
   //Q_OBJECT

    public:
    //! Default constructor
    ZMQMessageHandler();
    
    //! Regex for any integer between 0 and 255 (0/1 followed by two figures 0-9, OR 2 followed by a figure 0-4 and another 0-9, OR 25 and a figure 0-5)
    inline static const QString ipRangeRegex = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    //! Regex for an ip address (4 ints between 0-255 interleaved by a ".")
    inline static const QRegularExpression ipRegex = QRegularExpression("^" + ipRangeRegex
                                                                   + "(\\." + ipRangeRegex + ")"
                                                                   + "(\\." + ipRangeRegex + ")"
                                                                   + "(\\." + ipRangeRegex + ")$");

    static QTimer* localTick;   //!< Timer needed to keep sender and receiver in sync

    //! Handles pause/resume specifically for the AnimationMessageSender subclass
    /*!
    * It's necessary to have the wait condition in the superclass in order to have access to it and unlock it every time the localTimeStamp is incremented.
    * Every thread should have a dedicated wait condition in order to be able to wake them in the desired order.
    */
    static QWaitCondition* sendFrameWaitCondition;

    //! Request this process to start working
    /* \todo To be implemented in concrete class */
    virtual void requestStart() = 0; 

    //! Request this process to stop working
    /* \todo To be implemented in concrete class */
    virtual void requestStop() = 0;

    //! Sets the debug state
    /*!
     * @param _debugState The bool to be set as the new debug state
     */
    void setDebugState(bool _debugState) {
        _debug = _debugState;
    }

    //! Resumes the thread that is sending frames
    /*!
     * Sets \c _paused to false and wakes the specific thread that is waiting on the sendFrameWaitCondition
     */
    void resumeSendFrames();

    //! Pauses main loop execution
    /*!
     * Sets \c _paused to true
     */
    void pause() {
        mutex.lock();
        _paused = true;
        mutex.unlock();
    }

    //! Enumeration of Message Types allowed by TRACER
    /*!
     * Enumeration common to every TRACER element (DataHub and other clients alike).
     * Enum Value      | Int Value | Description                                                               |
     * --------------: | :-------: | :------------------------------------------------------------------------ |
     * PARAMETERUPDATE | 0         | The message contains values for a parameter of an object in the scene     |
     * LOCK            | 1         | The message locks all attributes of an object in the scene                |
     * SYNC            | 2         | The message contains data for syncronising two TRACER applications        |
     * PING            | 3         | The message contains/requests? a ping                                     |
     * RESENDUPDATE    | 4         | The message asks for a parameter update to be sent again                  |
     * UNDOREDO        | 5         | The message asks for a modification in a scene has to be undone/redone    |
     * RESETOBJECT     | 6         | The message asks for all modifications to an object to be reverted        |
     * DATAHUB         | 7         | ?                                                                         |
     * EMPTY           | 255       | Empty message (default)                                                   |
     */
    enum MessageType {
        PARAMETERUPDATE, LOCK, // node
        SYNC, PING, RESENDUPDATE, // sync
        UNDOREDOADD, RESETOBJECT, // undo redo
        DATAHUB, // DataHub
        EMPTY = 255
    };

    //! Enumeration of Message Types allowed by TRACER
    /*!
     * Enumeration common to every TRACER element (DataHub and other clients alike).
     * Enum Value | Int Value | Description                                                                           |
     * ---------: | :-------: | :------------------------------------------------------------------------------------ |
     * NONE       | 0         | Empty message                                                                         |
     * ACTION     | 1         | Action/command message                                                                |
     * BOOL       | 2         | The message contains a boolean value                                                  |
     * INT        | 3         | The message contains an integer scalar value                                          |
     * FLOAT      | 4         | The message contains a floating point scalar value                                    |
     * VECTOR2    | 5         | The message contains a vector with two floats                                         |
     * VECTOR3    | 6         | The message contains a vector with three floats                                       |
     * VECTOR4    | 7         | The message contains a vector with four floats                                        |
     * QUATERNION | 8         | The message contains a vector with four floats, interpreted and used as a quaternion  |
     * COLOR      | 9         | The message contains a vector with four floats, interpreted and used as a RGBA colour |
     * STRING     | 10        | The message contains a string of variable size                                        |
     * LIST       | 11        | The message contains a list of ??? of variable size                                   |
     * UNKNOWN    | 100       | Uninitialised message                                                                 |
     */
    enum ParameterType : byte {
        NONE, ACTION, BOOL,                     // Generic
        INT, FLOAT,                             // Scalar
        VECTOR2, VECTOR3, VECTOR4, QUATERNION,  // Vectors
        COLOR, STRING, LIST,                    // Other Data Structures
        UNKNOWN = 100
    };

    //! Returns the size size of each paramter in bytes
    /*!
     * @param[in]   parameterType   The type of parameter
     * @returns     The size of the Update Message payload in bytes
     */
    static constexpr byte getParameterDimension(ParameterType parameterType) {
        return parameterDimension[parameterType];
    }

    //! Increments the local timestamp
    /*!
    * Increments the local timestamp with the timing defined by the playback framerate (default is every ~17 msec = 60Hz).
    * The timestamp always stays in the range 0-bufferSize (by default equals to 120).
    */
    static void increaseTimeStamp() {
        localTimeStamp++;
        localTimeStamp = localTimeStamp % bufferSize;

        sendFrameWaitCondition->wakeAll();
    }

    //! Gets the list of available IP Addresses
    static QList<QHostAddress> getIPList() {
        return ZMQMessageHandler::ipList;
    }

    //! Returns the currently selected IP Address
    /*!
     * Static member function because it is supposed to return the same value for all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application
     */
    static QString getOwnIP() {
        return ZMQMessageHandler::ownIP;
    }

    //! Returns the current Client ID of the AnimHost Qt Application
    /*!
     * Static member function because it is supposed to return the same value for all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application.
     * @returns the last byte/octet of the IPv4 address in decimal format
     */
    static byte getOwnID() {
        qsizetype lastDotPos = 0; // qsizetype = int64

        if (ZMQMessageHandler::ownIP.contains(".")) {
          lastDotPos = ZMQMessageHandler::ownIP.lastIndexOf("."); // saving the index of the last occurence of a dot in the string
          return ZMQMessageHandler::ownIP.sliced(lastDotPos + 1).toUShort(); // everyithing after the last dot is considered the client's own ID (byte = ushort = uchar = uint8)
        } else {
            return 0;
        }
    }

    //! Sets the ID of the client that is supposed to receive the update
    /*!
     * Static member function because it is supposed to update the target Client ID for all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application
     */
    static void setTargetSceneID(byte _targetSceneID) {
        ZMQMessageHandler::targetSceneID = _targetSceneID;
    }

    //! Returns the ID of the client that is supposed to receive the update
    /*!
     * Static member function because it is supposed to return the same value for all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application.
     */
    static byte getTargetSceneID() {
        return ZMQMessageHandler::targetSceneID;
    }

    //! Sets the size of the client's buffer, to which the message will be written
    /*!
     * Static member function because it is supposed to update the size of the target buffer for all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application
     */
    static void setBufferSize(byte _bufferSize) {
        ZMQMessageHandler::bufferSize = _bufferSize;
    }

    //! Returns the size of the client's buffer, to which the message will be written
    /*!
     * Static member function because it is supposed to return the same value for all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application.
     */
    static byte getBufferSize() {
        return ZMQMessageHandler::bufferSize;
    }

    //! Sets the frame rate of the animation to be sent (default is 60 fps)
    /*!
     * Static member function because it is supposed to update the source-animation frame rate throughout all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application.
     * The frame rate is used in the [AnimHostMessageSender](@ref AnimHostMessageSender) class in order to compute the timing for sending poses out.
     */
    static void setAnimFrameRate(byte _animFR) {
        ZMQMessageHandler::animFrameRate = _animFR;
    }

    //! Returns the frame rate of the animation to be sent (default is 60 fps)
    /*!
     * Static member function because it is supposed to update the source-animation frame rate throughout all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application.
     */
    static byte getAnimFrameRate() {
        return ZMQMessageHandler::animFrameRate;
    }

    //! Sets the frame rate of the rendering application (default is 60 fps)
    /*!
     * Static member function because it is supposed to update the target application frame rate throughout all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application.
     * The frame rate is used in the [AnimHostMessageSender](@ref AnimHostMessageSender) class in order to compute the timing for sending poses out
     * and the eventual sampling rate.
     */
    static void setPlaybackFrameRate(byte _renderFR) {
        ZMQMessageHandler::playbackFrameRate = _renderFR;
    }

    //! Returns the frame rate of the rendering application (default is 60 fps)
    /*!
     * Static member function because it is supposed to update the rendering/receiving application frame rate throughout all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application.
     */
    static byte getPlaybackFrameRate() {
        return ZMQMessageHandler::playbackFrameRate;
    }

    //! Sets the local current timestamp
    /*!
     * Static member function because it is supposed to update the timestamp throughout all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application.
     * The timestamp is used in the [AnimHostMessageSender](@ref AnimHostMessageSender) class in order to write the next message in the right place in the buffer and
     * it can be updated from the [TickReceiver](@ref TickReceiver), for example.
     */
    static void setLocalTimeStamp(byte _newTS) {
        ZMQMessageHandler::localTimeStamp = _newTS % ZMQMessageHandler::bufferSize; // ensuring that the timestamp is in the interval 0-120
    }

    //! Returns the local current timestamp
    /*!
     * Static member function because it is supposed to update the rendering/receiving application frame rate throughout all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application.
     */
    static byte getLocalTimeStamp() {
        return ZMQMessageHandler::localTimeStamp;
    }

    //! Updates the currently selected IP address
    /*!
     * Updates the currently selected IP address, called when the selection in the drop-down menu in the UI of the TracerUpdateSender plugin changes
     * Static member function because it is supposed to update the Ip address for all instances of \c ZMQMessageHandler subclasses,
     * which can be created from different plugins in the Qt Application
     */
    static void setIPAddress(QString newIPAddress) {
        if (ipRegex.match(newIPAddress).isValid()) {
            ZMQMessageHandler::ownIP = newIPAddress;
            qDebug() << "New IP Address set!";
        } else {
            qDebug() << "Invalid IP Address" << newIPAddress;
        }
    }

    //! Creates a new 0MQ formatted message
    /*!
     * Creates a 0MQ message, with a new header and overwrites the member variable \c message with the new one
     * \sa message
     * \param[in]   timestamp   The timestamp of the message
     * \param[in]   messageType The type of the mesage
     * \param[in]   body        The body of the message, with one or more concatenated payloads (depending on \c messageType)
     */
    void createNewMessage(byte timestamp, ZMQMessageHandler::MessageType messageType, QByteArray* body);

    protected:
    
    //! List of available IP addresses
    static QList<QHostAddress> ipList;

    //! Client's own IP address
    static QString ownIP;

    //! ID of the TRACER client that is supposed to receive the messages
    static byte targetSceneID;

    static int bufferSize;          //!< Size of the receiving buffer (default = 120)
    static int animFrameRate;       //!< Frame rate of the loaded or generated animation (default = 60fps)
    static int playbackFrameRate;   //!< Frame rate used by the rendering application (default = 60fps)
    static int localTimeStamp;      //!< Local current timestamp, where the next message will be placed in the receiver's buffer
    
    //! If true, the process gets stopped
    /*!
    * Boolean used to keep track of the state of the main loop. If true, it gets interrupted and cleanup operations are triggered
    */
    bool _stop = true;

    //! Shall debug messages be printed
    bool _debug = false;

    //! If true, the process is running
    /*!
    * Boolean used to keep track of the state of the main loop. If true, the process is running
    */
    bool _working = false;

    //! If true, the process is running but paused
    /*!
    * Boolean used to keep track of the state of the main loop. If true, the process is interrupted but ready to be resumed
    */
    bool _paused = false;

    //! Protect access to variables that affect the execution state
    /*!
    * It avoids simultaneous changes to the executionstate variables from multiple processes
    */
    QMutex mutex;
    QMutex m_pauseMutex;

    //! 0MQ context pointer
    zmq::context_t* context = nullptr;

    //! The message of this instance of \c ZMQMessageHandler
    /*!
    * The message will be built, filled and overwritten according to the external calls from the Qt Application
    */
    QByteArray* message = new QByteArray();

    //! Message used for synchronising different clients
    /*!
    * It consists of exclusively of an TRACER message header (it doesn't have a body)
    */
    byte syncMessage[3] = { 0, 0, MessageType::EMPTY };

    //! Map of last states
    QMap<QByteArray, QByteArray> objectStateMap;

    //! Map of ping timings
    QMap<byte, unsigned int> pingMap;

    //! Map of last states
    QMultiMap<byte, QByteArray> lockMap;

    //! The local elapsed time in seconds since object has been created.
    unsigned int m_time = 0;

    //! Maximum time spent waiting for a ping response
    static const unsigned int m_pingTimeout = 4;

    //! Array storing the size of the various parameter types (in bytes) for ease of access
    /*!
    * Value      | Size in bytes
    * ---------: | :------------
    * NONE       | 0
    * ACTION     | 1
    * BOOL       | 4
    * INT        | 4
    * FLOAT      | 4
    * VECTOR2    | 8
    * VECTOR3    | 12
    * VECTOR4    | 16
    * QUATERNION | 16
    * COLOR-RGBA | 16
    * STRING     | 100
    * LIST       | 100
    * UNKNOWN    | 100
    */
    static constexpr byte parameterDimension[13] = {
        0, 1, sizeof(std::int32_t),
        sizeof(std::int32_t), sizeof(float),
        sizeof(float)*2, sizeof(float)*3, sizeof(float)*4, sizeof(float)*4,
        sizeof(float)*4, 100, 100, 100 };
    
    //! Utility function to convert a char value to a short scalar
    /*!
    * This function is used when receiving a message as a sequence of bytes (which are read as unsigned char),
    * converting them to short scalars makes debugging the messages more convenient
    */
    const short CharToShort(const char* buf) const {
        short val;
        std::memcpy(&val, buf, 2);
        return val;
    }

    signals :
    //! Signal emitted when process is finished
    void stopped();

    public slots:
    //! Main function for executing operations. Called from Qt Application
    virtual void run() {};

    protected slots:
    
    //! Populates/overwrites the sync message
    /*!
    * The sync message will always have the following structure:
    * 0. current ClientID of the AnimHost application
    * 1. current timestamp
    * 2. message type \c MessageType::SYNC
    */
    void createSyncMessage(int time) {
        syncMessage[0] = getOwnID();
        syncMessage[1] = time;
        syncMessage[2] = MessageType::SYNC;
        
        // increase local time for controlling client timeouts
        m_time++;
    }
};

#endif // ZMQMESSAGEHANDLER_H
