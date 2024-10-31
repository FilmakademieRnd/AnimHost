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
    qInfo() << "Try connecting to " << address;

    _ipAddr = serverIP;  // Save the server IP

    _clientID = ZMQMessageHandler::getOwnID();

    // Create a new ZeroMQ socket
    receiveSocket = new zmq::socket_t(*context, zmq::socket_type::sub);
    receiveSocket->connect(address.toLatin1().data());

    // Subscribe to all topics
    try {
        receiveSocket->setsockopt(ZMQ_SUBSCRIBE, "", 0);
        receiveSocket->setsockopt(ZMQ_RCVTIMEO, 0); 
        emit receiverStatus(0);
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
            emit receiverStatus(-1);
            break;
        }


        if (shouldStop){
			qDebug() << "Stopping TRACER Update Receiver";
            emit receiverStatus(-1);
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

		bool recievedMessage = receiveSocket->recv(&recvMsg, ZMQ_NOBLOCK);

        if (recievedMessage && recvMsg.size() > 0) {

            QByteArray msgArray = QByteArray((char*)recvMsg.data(), static_cast<int>(recvMsg.size())); // Convert message into explicit byte array
            
            deserializeMessage(msgArray);  // Deserialize the message

        }
        else {
            QThread::msleep(1); // Sleep for 1ms to prevent tight loop
        }
    }

    // Cleanup when stopping
    if (_stop) {
        qDebug() << "TRACER Update Receiver is stopping";
        receiveSocket->close();
        //QThread::msleep(100);  // Sleep for 100ms to allow the socket to close
        delete receiveSocket;
        receiveSocket = nullptr;
        qDebug() << "TRACER Update Receiver is stopped";
    }
}


void TRACERUpdateReceiver::deserializeMessage(QByteArray& rawMessageData)
{
    byte inClientID = rawMessageData[0];
    byte inTimeStamp = rawMessageData[1];
    MessageType inMessageType = static_cast<MessageType>(rawMessageData[2]);
    //qDebug() << inClientID << " "  << inMessageType;
    if(inClientID != _clientID){
	    
        switch(inMessageType){
			case MessageType::SYNC:
				qDebug() << "SYNC message received";
                _globalTimer->syncTimer(static_cast<int>(inTimeStamp));  // Sync the global timer with received sync time
				break;
            case MessageType::RPC:
                qDebug() << "RPC message received";
                emit receiverStatus(2);
				rawMessageData.remove(0, 3);  // Remove the first 3 bytes (Header)
				deserializeRPCMessage(rawMessageData);
                break;
			case MessageType::PARAMETERUPDATE:
				qDebug() << "PARAMETERUPDATE message received";
                emit receiverStatus(1);
                rawMessageData.remove(0, 3);  // Remove the first 3 bytes (Header)
                deserializeParameterUpdateMessage(rawMessageData);
				break;
            case MessageType::LOCK:
				qDebug() << "LOCKUPDATE message received";
				break;
			default:
				qDebug() << "Unknown message type received";
				break;
		}
	}
}

void TRACERUpdateReceiver::deserializeParameterUpdateMessage(const QByteArray& rawMessageData)
{

    QDataStream msgStream(rawMessageData);
    msgStream.setByteOrder(QDataStream::LittleEndian);
    msgStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    
    while(!msgStream.atEnd()){
        
        uint8_t sceneID;
        uint16_t objectID, paramID;
        ParameterType paramType;
        uint32_t length;
        
        msgStream >> sceneID;
        msgStream >> objectID;
        msgStream >> paramID;
        msgStream >> paramType;
        msgStream >> length;

        QByteArray data;
        data.resize(length-10);
        msgStream.readRawData(data.data(), length - 10); // -10 to account for the sceneID, objectID, paramID, paramType and length

        Q_EMIT parameterUpdateMessage(sceneID, objectID, paramID, paramType, data);
    }
}

void TRACERUpdateReceiver::deserializeRPCMessage(const QByteArray& rawMessageData)
{
    QDataStream msgStream(rawMessageData);
    msgStream.setByteOrder(QDataStream::LittleEndian);
    msgStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    while (!msgStream.atEnd()) {

        uint8_t sceneID;
        uint16_t objectID, paramID;
        ParameterType paramType;
        uint32_t lengthh;

        msgStream >> sceneID;
        msgStream >> objectID;
        msgStream >> paramID;
        msgStream >> paramType;
        msgStream >> lengthh;

        QByteArray data;
        data.resize(lengthh-10);
        msgStream.readRawData(data.data(), lengthh - 10); // -10 to account for the sceneID, objectID, paramID, paramType and length

        qDebug() << "SceneID: " << sceneID << " ObjectID: " << objectID << " ParamID: " << paramID << " ParamType: " << paramType;

        Q_EMIT rpcMessage(sceneID, objectID, paramID, paramType, data);
    }
}
