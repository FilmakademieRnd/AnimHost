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
 //! \file "TRACERUpdateReceiver.h"
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
#include "TRACERGlobalTimer.h"

#include <QMutex>
#include <QThread>


class TRACERPLUGINSHARED_EXPORT TRACERUpdateReceiver : public ZMQMessageHandler {

    Q_OBJECT

private:
    zmq::socket_t* receiveSocket = nullptr;     //!< Pointer to the instance of the socket that will listen to the messages

    std::shared_ptr<TRACERGlobalTimer> _globalTimer = nullptr; //!< Pointer to the global timer instance

    QString _ipAddr;    //!< IP address of the TRACER Server

    bool _restart = false;  //!< Flag to restart the receiver

    byte _clientID = 123; //PLACEHOLDER

public:

    //! Constructor
    /*!
    * \param[in]    m_debugState    Whether the class should print out debug messages
    * \param[in]    m_context       A pointer to the 0MQ Context instanciated by the \c TRACERUpdateReceiver
    */
    TRACERUpdateReceiver(bool m_debugState, zmq::context_t* m_context, std::shared_ptr<TRACERGlobalTimer> globalTimer) : _globalTimer(globalTimer) {
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;
    }

    //! Default destructor, closes connection and cleans up
    ~TRACERUpdateReceiver() {
        if(receiveSocket != nullptr) {
            try {
                receiveSocket->setsockopt(ZMQ_LINGER, 0);  // Set the linger period to 0
                receiveSocket->close();  // Gracefully close the ZeroMQ socket
            }
            catch (const zmq::error_t& e) {
                qWarning() << "Error closing socket: " << e.what();
            }
			//delete receiveSocket;
			receiveSocket = nullptr;
		}
        qDebug() << "Update Receiver destroyed";
    }

    //! Request this process to start working
    /*!
    * Sets \c _working to true, \c _stopped and \c _paused to false and creates a new publishing \c sendSocket for message broadcasting
    */
    void requestStart() override {
        //if (!_working) {
        //    mutex.lock();
        //    _working = true;
        //    _stop = false;
        //    _paused = false;
        //    qDebug() << "TRACER Update Message Receiver requested to start";// in Thread "<<thread()->currentThreadId();
        //    mutex.unlock();

        //    initializeUpdateReceiverSocket(_ipAddr);
        //}       
    }

    void requestStart(QString serverIP) {

		mutex.lock();
        if (!_working) {
            _working = true;
            _stop = false;
            _paused = false;
            _ipAddr = serverIP;
            
            mutex.unlock();
            qDebug() << "TRACER Update Message Receiver requested to start";
            QMetaObject::invokeMethod( this, "initializeUpdateReceiverSocket", Qt::QueuedConnection, Q_ARG(QString, serverIP));

            QMetaObject::invokeMethod(this, "runUpdateReciever", Qt::QueuedConnection);
        }
        else {
			mutex.unlock();
			qDebug() << "TRACER Update Message Receiver already running";
		}
	}

    void requestRestart(QString serverIP) {
        mutex.lock();
        if(_working){
			_restart = true;
            _ipAddr = serverIP;
            mutex.unlock();
            qDebug() << "TRACER Update Message Receiver requested to restart";
		}
        else {
		    mutex.unlock();
            qDebug() << "TRACER Update Message Receiver not running, starting instead";
            requestStart(serverIP);         
        }
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
            _working = false;
            mutex.unlock();
            qDebug() << "TRACER Update Message Receiver stopping";// in Thread "<<thread()->currentThreadId();
        }
		else {
			mutex.unlock();
			qDebug() << "TRACER Update Message Receiver already stopped";
		}
    }

private:

    void deserializeMessage(QByteArray& rawMessageData);

    void deserializeParameterUpdateMessage(const QByteArray& rawMessageData);

    void deserializeRPCMessage(const QByteArray& rawMessageData);   


private Q_SLOTS:

    /**
	 * @brief Initializes the update receiver socket
	 */
    void initializeUpdateReceiverSocket(QString serverIP = "127.0.0.1");

    /**
     * @brief Main loop, processes incoming messages from the TRACER clients & DataHub
     * Gets invoked when the TRACER Plugin is initialised and its corresbonding thread is started.
     * It opens a socket and listens to all the messages coming its way. 
     * If a SYNC message is received, it will update the global timer with the received timestamp.
     * If a PARAMETERUPDATE message is received, it will emit a signal with the objectID, paramID and the raw data.
     * If a LOCKUPDATE message is received, it will emit a signal with the objectID and the lock state.
     */
    void runUpdateReciever();


Q_SIGNALS:
    
    /**
     * @brief Signal emitted when a parameter update message is received. 
     * Passes along the objectID, paramID and the raw data for filtering and further processing
     * by the connected slots.
     * @param rawData The raw payload of the update message. Starts with the parameter value. Excluding the header.
     *
     */
    void parameterUpdateMessage(uint8_t sceneID, uint16_t objectID, uint16_t paramID, ParameterType paramType, const QByteArray rawData);

    /**
	 * @brief Signal emitted when a lock update message is received. 
	 * Passes along the objectID and the lock state for filtering and further processing
	 * by the connected slots.
	 */
    void lockUpdateMessage(uint16_t objectID, bool lockState);

    /**
    * @brief Signal emitted when a RPC message is received.
    */
    void rpcMessage(uint8_t sceneID, uint16_t objectID, uint16_t paramID, ParameterType paramType, const QByteArray rawData);

    void receiverStatus(int status);

    void stopped();

    void shutdown();
};
#endif // TICKRECEIVER_H
