#include "ZMQMessageHandler.h"

void ZMQMessageHandler::resume() {
    mutex.lock();
    _paused = false;
    waitCondition.wakeAll();
    mutex.unlock();
}