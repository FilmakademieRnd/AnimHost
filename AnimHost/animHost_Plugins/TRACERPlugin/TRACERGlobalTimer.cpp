#include "TRACERGlobalTimer.h"
#include <QDebug>
#include <QDateTime>

TRACERGlobalTimer::TRACERGlobalTimer(QObject *parent) : QObject(parent)
{
	// Start the timer with specified interval on the timer thread

		// Move the timer to the timer thread
	_timer = new QTimer(0);
	_timer->setTimerType(Qt::PreciseTimer);
	_timer->setSingleShot(false);
	_timer->setInterval(1000 / _playbackFrameRate);

	_timerThread = new QThread();


	// Start the timer thread
	_timerThread->start();

	_timer->moveToThread(_timerThread);

	// Connect the timer timeout signal to the onTimeout slot. This connection is direct, 
	// meaning that the slot is executed in the signalling thread.
	connect(_timer, &QTimer::timeout, this, &TRACERGlobalTimer::onTimeout, Qt::ConnectionType::DirectConnection);
}

TRACERGlobalTimer::~TRACERGlobalTimer()
{
	stopTimer();
	_timerThread->quit();
	_timerThread->wait();
}

void TRACERGlobalTimer::startTimer()
{
	if (!_timer->isActive()) {
		

		QMetaObject::invokeMethod(_timer, "start");

		qDebug() << "Timers thread: " << _timer->thread();
		qDebug() << "Timer thread: " << _timerThread;
		qDebug() << "main thread: " << this->thread();
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

	//qDebug() << "Syncing ... " << externalTimeStamp << " ... " << _localTimeStamp;

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
	QMutexLocker locker2(&_tickMutex);
	_localTimeStamp++;
	_localTimeStamp %= _bufferSize;


	////qDebug() << "Timer ticked at " << _localTimeStamp;
	////get the current precise system time
    //curr = QDateTime::currentMSecsSinceEpoch();
	////calculate the time difference between the current time and the last time the timer ticked
	//qint64 diff = curr - prev;
	//qDebug() << "Time difference: " << diff;
	//prev = curr;

	_tickWaitCondition.wakeAll();
	// Emit the timer tick signal
	emit timerTick();
}
