#include "TRACERGlobalTimer.h"

TRACERGlobalTimer::TRACERGlobalTimer(QObject *parent) : QObject(parent)
{
	// Move the timer to the timer thread
	_timer = new QTimer();
	_timer->setTimerType(Qt::PreciseTimer);
	_timer->setSingleShot(false);
	_timer->setInterval(1000 / _animFrameRate);
	_timer->moveToThread(&_timerThread);

	// Connect the timer timeout signal to the onTimeout slot
	connect(_timer, &QTimer::timeout, this, &TRACERGlobalTimer::onTimeout);

	// Start the timer thread
	_timerThread.start();
}

TRACERGlobalTimer::~TRACERGlobalTimer()
{
	stopTimer();
	_timerThread.quit();
	_timerThread.wait();
}

void TRACERGlobalTimer::startTimer(int interval)
{
	if (!_timer->isActive()) {
		// Start the timer with specified interval on the timer thread
		QMetaObject::invokeMethod(_timer, "start", Q_ARG(int, interval));
	}
}

void TRACERGlobalTimer::stopTimer()
{
	if (_timer->isActive()) {
		// Stop the timer on the timer thread
		QMetaObject::invokeMethod(_timer, "stop");
	}
}

void TRACERGlobalTimer::syncTimer(int externalTimeStamp)
{
	// Synchronize the timer on the timer thread to external timestamp
	QMutexLocker locker(&_timeStampMutex);

	if (std::abs(externalTimeStamp - _localTimeStamp) > 20) {
		// Set the new timestamp
		_localTimeStamp = externalTimeStamp;
	}
	
}

void TRACERGlobalTimer::waitOnTick()
{
	// Lock the mutex
	QMutexLocker locker(&_tickMutex);
	// Wait for the tick signal & wait unlocks the mutex during the wait
	_tickWaitCondition.wait(&_tickMutex);

}

int TRACERGlobalTimer::getLocalTimeStamp()
{
	QMutexLocker locker(&_timeStampMutex);
	return _localTimeStamp;
}

void TRACERGlobalTimer::onTimeout()
{
	// Increase the local time stamp
	QMutexLocker locker(&_timeStampMutex);
	_localTimeStamp++;
	_localTimeStamp %= _bufferSize;
	// Emit the timer tick signal
	emit timerTick();
}
