/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *    
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *    
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program; 
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */

 

//!
//! \file "TickReceiver.h"
//! \implements ZMQMessageHandler
//! \brief Class used to listen SYNC messages from TRACER clients
//! \author Francesco Andreussi
//! \version 0.5
//! \date 06.03.2024
//!
/*!
 * ###This class is instanced by the [TracerUpdateSenderPlugin](@ref TracerUpdateSenderPlugin) and is run in a subthread.
 * The class listens to SYNC messages from DataHub and the other TRACER clients, keeps the timestamps in check,
 * and notifies \c TracerUpdateSenderPlugin of the arrival of the message
 */

#ifndef TRACERUPDATERECEIVER_H
#define TRACERUPDATERECEIVER_H

#include "ZMQMessageHandler.h"
#include "TracerUpdateReceiverPlugin.h"

#include <QMutex>
#include <QThread>
//#include <nzmqt/nzmqt.hpp>

class TRACERUPDATERECEIVERPLUGINSHARED_EXPORT TracerUpdateReceiver : public ZMQMessageHandler {

    Q_OBJECT

    public:

    //! Default constructor
    TracerUpdateReceiver() {}

    //! Constructor
    /*!
    * \param[in]    m_debugState    Whether the class should print out debug messages
    * \param[in]    m_context       A pointer to the 0MQ Context instanciated by the \c TracerUpdateReceiverPlugin
    */
    TracerUpdateReceiver(bool m_debugState, zmq::context_t* m_context) {
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;
    }

    //! Default destructor, closes connection and cleans up
    ~TracerUpdateReceiver() {
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
        qDebug() << "AnimHost Update Message Receiver requested to start";// in Thread "<<thread()->currentThreadId();
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
            qDebug() << "AnimHost Update Message Receiver stopping";// in Thread "<<thread()->currentThreadId();
        }
        mutex.unlock();
    }

    private:
    zmq::socket_t* receiveSocket = nullptr;     //!< Pointer to the instance of the socket that will listen to the messages

    public Q_SLOTS:
    //! Main loop, executes all operations
    /*!
     * Gets triggered by the Qt Application UI thread, when the TracerUpdateSender Plugin is initialised.
     * It opens a socket and listens to all the messages coming its way. If a SYNC message is received, it reads it and notifies the UI thread.
     */
    void run() {
        // open socket
        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::sub);
        receiveSocket->connect(QString("tcp://127.0.0.1:5556").toLatin1().data());

        // In order to subscribe to every topic it is necessary to specify a option-lenght of 0 (ZERO) characters
        // Using only an empty string doesn't work because by default it will contain an END-OF-STRING character so its length will be 1 instead of 0
        receiveSocket->setsockopt(ZMQ_SUBSCRIBE, "", 0);

        zmq::message_t recvMsg;

        while (_working) {
            //qDebug() << "Waiting for PARAMETERUPDATE message...";
            //try to receive zeroMQ messages
            receiveSocket->recv(&recvMsg);

            if (recvMsg.size() > 0) {

                //qDebug() << "Checking message...";

                QByteArray msgArray = QByteArray((char*) recvMsg.data(), static_cast<int>(recvMsg.size())); // Convert message into explicit byte array
                MessageType msgType = static_cast<MessageType>(msgArray[2]);                                // Extract message type from byte array (always third byte)

                //if the message is of type PARAMETERUPDATE
                if (msgType == MessageType::PARAMETERUPDATE) {
                    qDebug() << "PARAMETERUPDATE message received";
                    msgArray.remove(0, 3); // Remove first 3 bytes (aka the Header)
                    mutex.lock();
                    processUpdateMessage(&msgArray);
                    mutex.unlock();
                }
            }
        }
    }

    Q_SIGNALS:
    void processUpdateMessage(QByteArray* updateMessage);    //!< Signal emitted when a SYNC message is received. Passes along the received timestamp
    void stopped();             //!< Signal emitted when process is finished
};
#endif // TICKRECEIVER_H
