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
    
    // Set the path to the PowerShell launcher script relative to the AnimHost executable location
    _trainingScriptPath = QApplication::applicationDirPath() + "/../../python/ml_framework/launch_training.ps1";
    
    // Connect process signals
    connect(_trainingProcess, &QProcess::readyReadStandardOutput, 
            this, &TrainingNode::onTrainingOutput);
    connect(_trainingProcess, &QProcess::readyReadStandardError,
            this, &TrainingNode::onTrainingError);
    connect(_trainingProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TrainingNode::onTrainingFinished);
    connect(_trainingProcess, &QProcess::errorOccurred,
            this, &TrainingNode::onTrainingProcessError);
    
    qDebug() << "TrainingNode created with Python script path:" << _trainingScriptPath;
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
        return 1;  // One StarkeConfig input port
    else
        return 0;  // No output ports
}

NodeDataType TrainingNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In && portIndex == 0) {
        return AnimNodeData<MLFramework::StarkeConfig>::staticType();
    }
    return NodeDataType{};
}

void TrainingNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    if (portIndex == 0) {
        // StarkeConfig input
        _configIn = std::static_pointer_cast<AnimNodeData<MLFramework::StarkeConfig>>(data);
        if (!_configIn.expired()) {
            qDebug() << "TrainingNode received StarkeConfig";

            // Update widget with new configuration
            if (_widget) {
                auto configData = _configIn.lock();
                if (configData && configData->getData()) {
                    _widget->setConfiguration(*configData->getData());
                }
            }
        } else {
            qDebug() << "TrainingNode: Invalid StarkeConfig data";
        }
    }
}

std::shared_ptr<NodeData> TrainingNode::processOutData(QtNodes::PortIndex port)
{
    return nullptr;  // No output data
}

bool TrainingNode::isDataAvailable()
{
    // Check if config data is available and valid
    if (_configIn.expired()) {
        return false;
    }

    if (auto configData = _configIn.lock()) {
        if (auto configPtr = configData->getData()) {
            return true;
        }
    }

    return false;
}

void TrainingNode::run()
{
    qDebug() << "TrainingNode run - Starting Python training process!";

    // Check for required config data using isDataAvailable()
    if (!isDataAvailable()) {
        qWarning() << "TrainingNode: No valid config data available, cannot start training";
        updateConnectionStatus("No Config", Qt::red);
        return;
    }

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

    // Get current config from input
    auto configData = _configIn.lock();
    auto configPtr = configData->getData();
    MLFramework::StarkeConfig currentConfig = *configPtr;

    // Save config to temporary file for Python script
    QString configPath = QApplication::applicationDirPath() + "/../../python/ml_framework/starke_model_config.json";
    QJsonObject configJson = currentConfig.toJson();
    QJsonDocument doc(configJson);
    QFile configFile(configPath);
    if (configFile.open(QIODevice::WriteOnly)) {
        configFile.write(doc.toJson());
        configFile.close();
        qDebug() << "Saved training config to:" << configPath;
    } else {
        qWarning() << "Failed to save config file:" << configPath;
    }

    // Update UI to show we're starting
    updateConnectionStatus("Starting...", QColor(255, 200, 50)); // Orange (like TRACER)
    
    // Start the PowerShell launcher script
    QString program = "powershell";
    QStringList arguments;
    arguments << "-ExecutionPolicy" << "Bypass" << "-File" << _trainingScriptPath;

    qDebug() << "Starting process:" << program << arguments;
    _trainingProcess->start(program, arguments);
}

QWidget* TrainingNode::embeddedWidget()
{
    if (!_widget) {
        _widget = new TrainingNodeWidget();

        // Apply current configuration if available
        if (!_configIn.expired()) {
            auto configData = _configIn.lock();
            if (configData && configData->getData()) {
                _widget->setConfiguration(*configData->getData());
            }
        }
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
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(trimmedLine.toUtf8(), &parseError);
            
            if (parseError.error != QJsonParseError::NoError) {
                qWarning() << "JSON parse error:" << parseError.errorString() << "in line:" << trimmedLine;
                continue;
            }
            
            MLFramework::TrainingMessage msg = MLFramework::TrainingMessage::fromJson(doc.object());
            updateFromMessage(msg);
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

void TrainingNode::onTrainingError()
{
    QByteArray data = _trainingProcess->readAllStandardError();
    QString errorOutput = QString::fromUtf8(data);
    
    if (!errorOutput.isEmpty()) {
        qDebug() << "Python error output:" << errorOutput.trimmed();
    }
}

void TrainingNode::onTrainingProcessError(QProcess::ProcessError error)
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

void TrainingNode::updateConnectionStatus(const QString& status, const QColor& signalColor)
{
    if (_widget) {
        _widget->updateConnectionStatus(status, signalColor);
    }
}

void TrainingNode::updateFromMessage(const MLFramework::TrainingMessage& msg)
{
    if (_widget) {
        _widget->updateFromMessage(msg);
    }
}
