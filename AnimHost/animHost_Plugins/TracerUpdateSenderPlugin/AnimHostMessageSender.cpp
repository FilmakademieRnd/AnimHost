#include "AnimHostMessageSender.h"

//targetHostID = _ipAddress.at(_ipAddress.size() - 1).digitValue();

#include <QThread>
#include <QDebug>
#include <iostream>

void AnimHostMessageSender::requestStart() {
    mutex.lock();
    _working = true;
    _stop = false;
    qDebug() << "AnimHost Message Sender requested to start";// in Thread "<<thread()->currentThreadId();
    mutex.unlock();

    emit startRequested();
}

void AnimHostMessageSender::requestStop() {
    mutex.lock();
    if (_working) {
        _stop = true;
        qDebug() << "AnimHost Message Sender stopping";// in Thread "<<thread()->currentThreadId();
    }
    mutex.unlock();
}

void AnimHostMessageSender::createSyncMessage(int time) {
    syncMessage[0] = targetHostID;
    syncMessage[1] = time;
    syncMessage[2] = MessageType::SYNC;

    // increase local time for controlling client timeouts
    m_time++;
}

void AnimHostMessageSender::run() {

    sendSocket = new zmq::socket_t(*context, zmq::socket_type::pub); // publisher socket
    sendSocket->connect(QString("tcp://" + ipAddress + ":5557").toLatin1().data());

    /*zmq::pollitem_t items[] = {
            { static_cast<void*>(*sendSocket), 0, ZMQ_POLLIN, 0 }
    };*/

    qDebug() << "Starting AnimHost Message Sender";// in Thread " << thread()->currentThreadId();

    while (true) {

        // checks if process should be aborted
        mutex.lock();
        bool stop = _stop;
        mutex.unlock();

        bool msgIsExternal = false;

        // SENDING MESSAGES
        zmq::message_t* tempMsg = new zmq::message_t();
        tempMsg->rebuild(message->data(), message->size());

        std::string debugOut;
        float* debugDataArray = static_cast<float*>(tempMsg->data());
        for (int i = 0; i < message->size() / sizeof(float); i++) {
            debugOut = debugOut + std::to_string(debugDataArray[i]) + " ";
        }
        

        qDebug() << "Message size: " << tempMsg->size();
        qDebug() << "Sending message: " << debugOut;
        sendSocket->send(*tempMsg);

        if (stop) {
            qDebug() << "Stopping AnimHost Message Sender";// in Thread "<<thread()->currentThreadId();
            break;
        }
    }

    // Set _working to false -> process cannot be aborted anymore
    mutex.lock();
    _working = false;
    mutex.unlock();

    qDebug() << "AnimHost Message Sender process stopped";// in Thread "<<thread()->currentThreadId();

    emit stopped();
}