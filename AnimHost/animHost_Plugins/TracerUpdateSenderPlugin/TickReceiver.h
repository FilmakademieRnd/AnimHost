
//!
//! \file "TickReceiver.h"
//! \implements ZMQMessageHandler
//! \brief Class used to listen SYNC messages from TRACER clients     
//! \author Francesco Andreussi
//! \version 0.5
//! \date 26.01.2024
//!
/*!
 * ###This class is instanced by the [TracerUpdateSenderPlugin](@ref TracerUpdateSenderPlugin) and is run in a subthread.
 * The class listens to SYNC messages from DataHub and the other TRACER clients, keeps the timestamps in check,
 * and notifies \c TracerUpdateSenderPlugin of the arrival of the message
 */

#ifndef TICKRECEIVER_H
#define TICKRECEIVER_H

#include "ZMQMessageHandler.h"
#include "TracerUpdateSenderPlugin.h"

#include <QMutex>
#include <QThread>
#include <nzmqt/nzmqt.hpp>

class TRACERUPDATESENDERPLUGINSHARED_EXPORT TickReceiver : public ZMQMessageHandler {

	Q_OBJECT

	public:

    //! Default constructor
	TickReceiver() {}

    //! Constructor
    /*!
    * \param[in]    m_TUSPlugin     The TracerUpdateSenderPlugin instance that calls the constructor
    * \param[in]    m_debugState    Whether the class should print out debug messages
    * \param[in]    m_context       A pointer to the 0MQ COntext instanciated by \c m_TUSPlugin
    */
    TickReceiver(TracerUpdateSenderPlugin* m_TUSPlugin, bool m_debugState, zmq::context_t* m_context) {
        TUSPlugin = m_TUSPlugin;
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;
    }

    //! Constructor
    /*!
    * \param[in]    m_debugState    Whether the class should print out debug messages
    * \param[in]    m_context       A pointer to the 0MQ COntext instanciated by \c m_TUSPlugin
    */
    TickReceiver(bool m_debugState, zmq::context_t* m_context) {
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;
    }

    //! Default destructor, closes connection and cleans up
	~TickReceiver() {
        receiveSocket->close();
    }

    //! Request this process to start working
    /*!
    * Sets \c _working to true, \c _stopped and \c _paused to false and creates a new publishing \c sendSocket for message broadcasting
    */
    void requestStart() override {
        mutex.lock();
        _working = true;
        _stop = false;
        _paused = false;
        qDebug() << "AnimHost Tick Receiver requested to start";// in Thread "<<thread()->currentThreadId();
        mutex.unlock();
    }

    //! Request this process to stop working
    /*!
    * Sets \c _stopped to true and \c _paused to false. -Should- close \c sendSocket and cleanup
    */
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
    zmq::socket_t* receiveSocket = nullptr;         //!< Pointer to the instance of the socket that will listen to the messages
    TracerUpdateSenderPlugin* TUSPlugin = nullptr;  //!< Pointer to the instance of TracerUpdateSenderPlugin that owns this TickReceiver

	public Q_SLOTS:
    //! Main loop, executes all operations
    /*!
     * Gets triggered by the Qt Application UI thread, when the TracerUpdateSender Plugin is initialised.
     * It opens a socket and listens to all the messages coming its way. If a SYNC message is received, it reads it and notifies the UI thread.
     */
    void run() {
        // open socket
        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::sub);
        receiveSocket->connect(QString("tcp://" + ZMQMessageHandler::ownIP + ":5556").toLatin1().data());
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
    void tick(int syncTime);    //!< Signal emitted when a SYNC message is received. Passes along the received timestamp
    void stopped();             //!< Signal emitted when process is finished
};
#endif // TICKRECEIVER_H
