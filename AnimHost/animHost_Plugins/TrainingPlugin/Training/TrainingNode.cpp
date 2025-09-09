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

#include "TrainingNode.h"
#include "TrainingNodeWidget.h"
#include <QApplication>

TrainingNode::TrainingNode()
{
    // Initialize UI
    _widget = nullptr;
    
    // Initialize process management
    _trainingProcess = new QProcess(this);
    
    // Set the path to the Python script (relative to AnimHost root)
    _pythonScriptPath = QApplication::applicationDirPath() + "/../../python/ml_framework/mock_training.py";
    
    // Connect process signals
    connect(_trainingProcess, &QProcess::readyReadStandardOutput, 
            this, &TrainingNode::onTrainingOutput);
    connect(_trainingProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TrainingNode::onTrainingFinished);
    connect(_trainingProcess, &QProcess::errorOccurred,
            this, &TrainingNode::onTrainingError);
    
    qDebug() << "TrainingNode created with Python script path:" << _pythonScriptPath;
}

TrainingNode::~TrainingNode()
{
    // Terminate training process if still running
    if (_trainingProcess && _trainingProcess->state() != QProcess::NotRunning) {
        _trainingProcess->terminate();
        if (!_trainingProcess->waitForFinished(3000)) {
            _trainingProcess->kill();
        }
    }
    qDebug() << "~TrainingNode()";
}

unsigned int TrainingNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;  // No custom input ports
    else
        return 0;  // No output ports
}

NodeDataType TrainingNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    return NodeDataType{};
}

void TrainingNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    // No custom input ports - framework handles RunSignal automatically
    // This method won't be called since nDataPorts returns 0 for input
    qDebug() << "TrainingNode processInData called unexpectedly";
}

std::shared_ptr<NodeData> TrainingNode::processOutData(QtNodes::PortIndex port)
{
    return nullptr;  // No output data
}

bool TrainingNode::isDataAvailable() 
{
    // We can always run - either from button click or run signal
    return true;
}

void TrainingNode::run()
{
    qDebug() << "TrainingNode run - Starting Python training process!";
    
    // Check if process is already running
    if (_trainingProcess->state() != QProcess::NotRunning) {
        qDebug() << "Training process already running, terminating first...";
        _trainingProcess->terminate();
        _trainingProcess->waitForFinished(3000);
    }
    
    // Reset progress
    if (_widget) {
        _widget->resetProgress();
    }
    
    // Update UI to show we're starting
    updateConnectionStatus("Starting...", QColor(255, 200, 50)); // Orange (like TRACER)
    
    // Start the Python training script
    QString program = "python";
    QStringList arguments;
    arguments << _pythonScriptPath;
    
    qDebug() << "Starting process:" << program << arguments;
    _trainingProcess->start(program, arguments);
}

QWidget* TrainingNode::embeddedWidget()
{
    if (!_widget) {
        _widget = new TrainingNodeWidget();
    }
    
    return _widget;
}

void TrainingNode::onTrainingOutput()
{
    QByteArray data = _trainingProcess->readAllStandardOutput();
    QString output = QString::fromUtf8(data);
    if (output.isEmpty() && !data.isEmpty()) {
        qWarning() << "Failed to decode training process message as UTF-8 data.";
        return;
    }
    
    // Process each line separately (in case multiple JSON objects are received)
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (!trimmedLine.isEmpty()) {
            qDebug() << "Received JSON:" << trimmedLine;
            
            // Parse JSON document
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(trimmedLine.toUtf8(), &parseError);
            
            if (parseError.error != QJsonParseError::NoError) {
                qWarning() << "JSON parse error:" << parseError.errorString() << "in line:" << trimmedLine;
                continue;
            }
            
            updateFromMessage(doc.object());
        }
    }
}

void TrainingNode::onTrainingFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Training process finished with exit code:" << exitCode << "status:" << exitStatus;
    
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        updateConnectionStatus("Completed", QColor(50, 255, 50)); // Bright green (like TRACER)
    } else {
        updateConnectionStatus("Failed", Qt::red);
    }
}

void TrainingNode::onTrainingError(QProcess::ProcessError error)
{
    QString errorString;
    switch (error) {
        case QProcess::FailedToStart:
            errorString = "Failed to start Python process";
            break;
        case QProcess::Crashed:
            errorString = "Python process crashed";
            break;
        case QProcess::Timedout:
            errorString = "Python process timed out";
            break;
        default:
            errorString = "Unknown process error";
            break;
    }
    
    qDebug() << "Process error:" << errorString;
    updateConnectionStatus("Error", Qt::red);
}

void TrainingNode::updateConnectionStatus(const QString& status, const QColor& lightColor)
{
    if (_widget) {
        _widget->updateConnectionStatus(status, lightColor);
    }
}

void TrainingNode::updateFromMessage(const QJsonObject& obj)
{
    if (_widget) {
        _widget->updateFromMessage(obj);
    }
}