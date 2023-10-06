#include "AnimHostMessageSender.h"

//targetHostID = _ipAddress.at(_ipAddress.size() - 1).digitValue();

#include <QThread>
#include <QDebug>
#include <iostream>

void AnimHostMessageSender::requestStart() {
    mutex.lock();
    _working = true;
    _stop = false;
    _paused = false;
    qDebug() << "AnimHost Message Sender requested to start";// in Thread "<<thread()->currentThreadId();
    mutex.unlock();
}

void AnimHostMessageSender::requestStop() {
    mutex.lock();
    if (_working) {
        _stop = true;
        _paused = false;
        //_working = false;
        qDebug() << "AnimHost Message Sender stopping";// in Thread "<<thread()->currentThreadId();
    }
    mutex.unlock();
}

void AnimHostMessageSender::setMessage(zmq::message_t* msg) {
    qDebug() << "Setting message of size " << msg->size();
    message.copy(msg);
    qDebug() << "Setting message of size " << message.size();

}

void AnimHostMessageSender::run() {

    sendSocket = new zmq::socket_t(*context, zmq::socket_type::pub); // publisher socket
    sendSocket->connect(QString("tcp://" + ipAddress + ":5557").toLatin1().data());

    /*zmq::pollitem_t items[] = {
            { static_cast<void*>(*sendSocket), 0, ZMQ_POLLIN, 0 }
    };*/

    qDebug() << "Starting AnimHost Message Sender";// in Thread " << thread()->currentThreadId();

    zmq::message_t* tempMsg = new zmq::message_t();
    int type;
    size_t type_size = sizeof(type);
    int count = 0;
    while (_working) {

        // checks if process should be aborted
        mutex.lock();
        bool stop = _stop;
        if (_paused)
            waitCondition.wait(&mutex); // in this place, your thread will stop to execute until someone calls resume
        else
            _paused = true; // Execute once and pause at the beginning of next iteration of while-loop
        mutex.unlock();

        bool msgIsExternal = false;

        // SENDING MESSAGES
        tempMsg->rebuild(message.data(), message.size());

        std::string debugOut;
        byte* debugDataArray = static_cast<byte*>(tempMsg->data());
        for (int i = 0; i < message.size() / sizeof(byte); i++) {
            debugOut = debugOut + std::to_string(debugDataArray[i]) + " ";
        }

        sendSocket->getsockopt(ZMQ_TYPE, &type, &type_size);

        QThread::msleep(1);

        qDebug() << "Message size: " << tempMsg->size();
        qDebug() << "Sending message: " << debugOut;
        int retunVal = sendSocket->send(*tempMsg);

        if (stop) {
            qDebug() << "Stopping AnimHost Message Sender";// in Thread "<<thread()->currentThreadId();
            break;
        }
        QThread::yieldCurrentThread();
    }

    // Set _working to false -> process cannot be aborted anymore
    mutex.lock();
    _working = false;
    mutex.unlock();

    qDebug() << "AnimHost Message Sender process stopped";// in Thread "<<thread()->currentThreadId();

    emit stopped();
}