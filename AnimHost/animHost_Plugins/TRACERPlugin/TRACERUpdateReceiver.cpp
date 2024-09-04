#include "TRACERUpdateReceiver.h"

void TRACERUpdateReceiver::initializeUpdateReceiverSocket(QString serverIP) {

    // Check if serverIp is empty
    if (serverIP.isEmpty()) {
        qDebug() << "Server IP is empty";
        return;
    }

    // Close and recreate the socket if already initialized
    if (receiveSocket) {
        receiveSocket->close();
        delete receiveSocket;
        receiveSocket = nullptr;
    }

    QString address = "tcp://" + serverIP + ":5556";
    qDebug() << "Try connecting to " << address;

    _ipAddr = serverIP;  // Save the server IP

    // Create a new ZeroMQ socket
    receiveSocket = new zmq::socket_t(*context, zmq::socket_type::sub);
    receiveSocket->connect(address.toLatin1().data());

    // Subscribe to all topics
    try {
        receiveSocket->setsockopt(ZMQ_SUBSCRIBE, "", 0);
        receiveSocket->setsockopt(ZMQ_RCVTIMEO, 1000);  // Set a receive timeout of 1000ms (1 second)
    }
    catch (zmq::error_t& e) {
        qDebug() << "Error initializing socket: " << e.what();
        receiveSocket->close();
        delete receiveSocket;
        receiveSocket = nullptr;
    }

}

void TRACERUpdateReceiver::runUpdateReciever() {   
    while (_working) {
        mutex.lock();
        bool shouldStop = _stop;
        bool shouldRestart = _restart;
        mutex.unlock();

        // Ensure a valid socket exists
        if (!receiveSocket) {
            qDebug() << "No socket available";
            _working = false; // reset working flag, so start can be requested again
            break;
        }


        if (shouldStop){
			qDebug() << "Stopping TRACER Update Receiver";
			break;
		}

        if(shouldRestart){
			qDebug() << "Restarting TRACER Update Receiver";
            mutex.lock();
			_restart = false; // Reset the restart flag
            mutex.unlock();

            // Close and reinitialize the socket without calling the runUpdateReceiver function again
			initializeUpdateReceiverSocket(_ipAddr);
            continue;
		}

		// Receive message
		zmq::message_t recvMsg;

		bool recievedMessage = receiveSocket->recv(&recvMsg);

        if (recievedMessage && recvMsg.size() > 0) {

            QByteArray msgArray = QByteArray((char*)recvMsg.data(), static_cast<int>(recvMsg.size())); // Convert message into explicit byte array
            MessageType msgType = static_cast<MessageType>(msgArray[2]);                                // Extract message type from byte array (always third byte)

            if (msgType == MessageType::SYNC) {
                const int syncTime = static_cast<int>(msgArray[1]);  // Extract sync time (2nd byte)
                _globalTimer->syncTimer(syncTime);  // Sync the global timer with received sync time
            }
            else if (msgType == MessageType::PARAMETERUPDATE) {
                qDebug() << "PARAMETERUPDATE message received";
                msgArray.remove(0, 3);  // Remove the first 3 bytes (Header)
                mutex.lock();
                // Process the parameter update (method implementation required)
                mutex.unlock();
            }

        }
        else {
            QThread::msleep(1); // Sleep for 1ms to prevent busy waiting
        }
    }

    // Cleanup when stopping
    if (_stop) {
        qDebug() << "TRACER Update Receiver is stopping";
        receiveSocket->close();
        delete receiveSocket;
        receiveSocket = nullptr;
    }
}
