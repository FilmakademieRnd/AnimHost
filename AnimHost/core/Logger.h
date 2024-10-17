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

 
#ifndef LOGGER_H
#define LOGGER_H

#include "animhostcore_global.h"

#include <QDebug>
#include <QFile>
#include <QHash>
#include <QMutex>




class  ANIMHOSTCORESHARED_EXPORT Logger {

private:
	


	static QFile* logFile; //!< @brief The file to write the log to

	static bool isInitialized; //!< @brief Flag to check if the logger has been initialized

	static QHash<QtMsgType, QString> contextNames; //!< @brief Types of context messages

	static QHash<QtMsgType, QString> colorCodes; //!< @brief Color codes for the different message types

	static const QString resetCode; //!< @brief Reset code for the color

	static QtMessageHandler defaultHandler; //!< @brief Default message handler

	static QMutex mutex; //!< @brief Mutex to lock the logger
public:
	
	/**
	 * @brief Initializes the logger
	 * 
	 */
	static void Initialize();


	/**
	 * @brief Cleanup the logger
	 * 
	 */
	static void Cleanup();


	/**
	 * @brief Outputs the message to the log file
	 *
	 * @param type The type of the message
	 * @param context The context of the message
	 * @param msg The message to output
	 */
	static void messageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg);

};

#endif  // LOGGER_H
