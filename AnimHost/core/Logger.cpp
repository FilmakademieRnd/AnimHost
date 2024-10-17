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

 
#include "Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QObject>

QFile* Logger::logFile = Q_NULLPTR;

QMutex Logger::mutex; 

bool Logger::isInitialized = false;

QHash<QtMsgType, QString> Logger::contextNames = {
	{QtMsgType::QtDebugMsg, "DEBUG"},
	{QtMsgType::QtInfoMsg, "INFO"},
	{QtMsgType::QtWarningMsg, "WARNING"},
	{QtMsgType::QtCriticalMsg, "CRITICAL"},
	{QtMsgType::QtFatalMsg, "FATAL"}
};

QHash<QtMsgType, QString> Logger::colorCodes = {
	{QtDebugMsg, "\033[36m"},    // Cyan
	{QtInfoMsg, "\033[32m"},     // Green
	{QtWarningMsg, "\033[33m"},  // Yellow
	{QtCriticalMsg, "\033[31m"}, // Red
	{QtFatalMsg, "\033[35m"}     // Magenta
};

const QString Logger::resetCode = "\033[0m";

QtMessageHandler Logger::defaultHandler = nullptr;

void Logger::Initialize()
{
	if (isInitialized)
	{
		return;
	}

	logFile = new QFile;
	logFile->setFileName("./LogOutput.txt");
	logFile->open(QIODevice::Append | QIODevice::Text);


	// Redirect logs to messageOutput
	defaultHandler = qInstallMessageHandler(Logger::messageOutput);

	// Clear the log file
	logFile->resize(0);

	Logger::isInitialized = true;
}

void Logger::Cleanup()
{
	if (logFile != Q_NULLPTR)
	{
		logFile->close();
		delete logFile;
	}
}

void Logger::messageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	QMutexLocker locker(&mutex);

	QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
	QString contextName = contextNames.value(type, "UNKNOWN");
	QString fileName = QString(context.file).section('\\', -1);
	QString functionName = QString(context.function).section('(', -2, -2).section(' ', -1).section(':', -1);

	QString formattedMessage = QString("%1 || %2 | %3 %4 %5 || %6\n")
		.arg(timestamp)
		.arg(contextName)
		.arg(context.line)
		.arg(fileName)
		.arg(functionName)
		.arg(msg);

	// Write to log file
	if (logFile && logFile->isOpen())
	{
		logFile->write(formattedMessage.toLocal8Bit());
		logFile->flush();
	}

	// Print to console with colored formatting
	QString colorCode = colorCodes.value(type, resetCode);

	QString formattedConsoleMessage = QString("%1 || %2")
		.arg(colorCode + contextName + resetCode)
		.arg(msg);

	QString consoleMessage = colorCode + formattedConsoleMessage + resetCode;

	// Call the default handler if it exists
	if (defaultHandler)
	{
		defaultHandler(type, context, consoleMessage);
	}
}
