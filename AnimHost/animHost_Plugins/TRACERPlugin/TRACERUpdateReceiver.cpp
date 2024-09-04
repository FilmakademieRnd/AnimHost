#include "TRACERUpdateReceiver.h"

void TRACERUpdateReceiver::initializeUpdateReceiverSocket(QString serverIP) {

    //check if serverIp is empty
    if (serverIP.isEmpty()) {
		qDebug() << "Server IP is empty";
		return;
	}

    //check if the receiver is already working
    if (!receiveSocket) {

        QString address = "tcp://" + serverIP + ":5556";
        qDebug() << "Try connecting to " << address;

        _ipAddr = serverIP;

        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::sub);
        receiveSocket->connect(address.toLatin1().data());
       
        // In order to subscribe to every topic it is necessary to specify a option-lenght of 0 (ZERO) characters
        // Using only an empty string doesn't work because by default it will contain an END-OF-STRING character so its length will be 1 instead of 0
        try{
			receiveSocket->setsockopt(ZMQ_SUBSCRIBE, "", 0);
            receiveSocket->setsockopt(ZMQ_RCVTIMEO, 1);
		} catch (zmq::error_t e) {
			qDebug() << "Error initializing socket: " << e.what();

            receiveSocket->close();
		}
    }
    else {

        receiveSocket->close();

        QString address = "tcp://" + serverIP + ":5556";
        qDebug() << "Try connecting to " << address;

        _ipAddr = serverIP;
        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::sub);
        receiveSocket->connect(address.toLatin1().data());
        try {
            receiveSocket->setsockopt(ZMQ_SUBSCRIBE, "", 0);
            receiveSocket->setsockopt(ZMQ_RCVTIMEO, 1);
        }
        catch (zmq::error_t e) {
            qDebug() << "Error initializing socket: " << e.what();

            receiveSocket->close();
        }
    }

    runUpdateReciever();
}

void TRACERUpdateReceiver::runUpdateReciever() {
    // open socket

    if(!receiveSocket) {
		qDebug() << "No socket available";
		return;
	}

    zmq::message_t recvMsg;

    while (_working){

        //try to receive zeroMQ messages        
        receiveSocket->recv(&recvMsg);

        if (recvMsg.size() > 0) {

            //qDebug() << "Checking message...";

            QByteArray msgArray = QByteArray((char*)recvMsg.data(), static_cast<int>(recvMsg.size())); // Convert message into explicit byte array
            MessageType msgType = static_cast<MessageType>(msgArray[2]);                                // Extract message type from byte array (always third byte)

            //if tick received with time tickTime
            if (msgType == MessageType::SYNC) { // Should we check also against Client ID?
                const int syncTime = static_cast<int>(msgArray[1]); // Extract sync time from message (always second byte)
                //qDebug() << "SYNC message received with time " << syncTime;
                mutex.lock();
                //tick(syncTime);
                _globalTimer->syncTimer(syncTime);
                mutex.unlock();
            }

            //if the message is of type PARAMETERUPDATE
            if (msgType == MessageType::PARAMETERUPDATE) {
                qDebug() << "PARAMETERUPDATE message received";
                msgArray.remove(0, 3); // Remove first 3 bytes (aka the Header)
                mutex.lock();
                //processUpdateMessage(&msgArray);
                mutex.unlock();
            }
        }
    }
    
    if (_stop) {
        qDebug() << "TRACER Update Receiver process to be stopped";
        emit shutdown();
    }
    
    if (_restart) {
		qDebug() << "TRACER Update Receiver process to be restarted";
		_restart = false;
        initializeUpdateReceiverSocket(_ipAddr);
	}
    
}

void TRACERUpdateReceiver::stopUpdateReceiver()
{
}
