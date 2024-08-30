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


#ifndef TRACERGLOBALTIMER_H
#define TRACERGLOBALTIMER_H

#include "TRACERPlugin_global.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
#include <QMutex>




class TRACERPLUGINSHARED_EXPORT TRACERGlobalTimer : public QObject {
	Q_OBJECT

		QThread _timerThread;
	    QTimer* _timer;

		int _bufferSize = 120;
		int _animFrameRate = 60;

		int _localTimeStamp = 0;

		QWaitCondition _tickWaitCondition;
		QMutex _tickMutex;
		QMutex _timeStampMutex;


	public:
		TRACERGlobalTimer(QObject *parent = nullptr);
		~TRACERGlobalTimer();

		void startTimer(int interval);
		
		void stopTimer();


		/** 
		* Checks whether the internal timer of the application and the other client's timer are in sync.
		* @param    externalTime    the time of the client, with which the application is communicating
		*/
		void syncTimer(int externalTimeStamp);

		/**
		 * @brief Blocks the calling thread until the next timer tick.
		 *
		 * This method is used by worker threads to wait for the next tick event.
		 * It encapsulates the locking and waiting logic, ensuring that threads
		 * wait on the same condition and are woken up when the timer signals the tick.
		 *
		 * @note This method should be called from worker threads that need to synchronize
		 * their operations with the timer ticks. The method locks an internal mutex before
		 * waiting and releases it once the thread is woken up.
		 */
		void waitOnTick();

		int getLocalTimeStamp();

	signals:
	    /** 
		* @brief Signal emitted when the TRACER timer ticks.
	    */
		void timerTick();

	private slots:
		/**
		* @brief Slot connected to the timer timeout signal.
		*/
		void onTimeout(); 


	   
};

#endif // TRACERGLOBALTIMER_H
