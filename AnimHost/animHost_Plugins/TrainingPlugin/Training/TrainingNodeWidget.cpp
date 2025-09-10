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

#include "TrainingNodeWidget.h"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonObject>

TrainingNodeWidget::TrainingNodeWidget(QWidget* parent)
    : QWidget(parent)
    , _mainLayout(nullptr)
    , _connectionGroupBox(nullptr)
    , _connectionLayout(nullptr)
    , _signalLight(nullptr)
    , _statusLabel(nullptr)
    , _encoderGroupBox(nullptr)
    , _encoderProgressBar(nullptr)
    , _encoderTrainLossLabel(nullptr)
    , _controllerGroupBox(nullptr)
    , _controllerProgressBar(nullptr)
    , _controllerTrainLossLabel(nullptr)
{
    setupUI();
    applyStyles();
    
    // Initialize connection status
    updateConnectionStatus("Uninitialized", Qt::red);
}

void TrainingNodeWidget::setupUI()
{
    _mainLayout = new QVBoxLayout(this);
    
    // Create ML Framework Group Box
    _connectionGroupBox = new QGroupBox("ML Framework Status", this);
    _connectionLayout = new QHBoxLayout(_connectionGroupBox);
    
    _signalLight = new SignalLightWidget(_connectionGroupBox);
    _statusLabel = new QLabel("Uninitialized", _connectionGroupBox);
    _statusLabel->setAlignment(Qt::AlignVCenter);
    
    _connectionLayout->addWidget(_signalLight);
    _connectionLayout->addWidget(_statusLabel);
    _connectionLayout->addStretch();
    
    // Create Encoder Status Group Box
    _encoderGroupBox = new QGroupBox("Encoder Status", this);
    auto* trainingLayout = new QVBoxLayout(_encoderGroupBox);
    
    _encoderProgressBar = new ProgressWidget<int>("Epoch", 2, _encoderGroupBox);
    _encoderTrainLossLabel = new QLabel("Training Loss: --", _encoderGroupBox);
    
    trainingLayout->addWidget(_encoderProgressBar);
    trainingLayout->addWidget(_encoderTrainLossLabel);

    // Create Controller Status Group Box
    _controllerGroupBox = new QGroupBox("Controller Status", this);
    auto* controllerLayout = new QVBoxLayout(_controllerGroupBox);

    _controllerProgressBar = new ProgressWidget<int>("Epoch", 2, _controllerGroupBox);
    _controllerTrainLossLabel = new QLabel("Training Loss: --", _controllerGroupBox);

    controllerLayout->addWidget(_controllerProgressBar);
    controllerLayout->addWidget(_controllerTrainLossLabel);

    // Add group boxes to main layout
    _mainLayout->addWidget(_connectionGroupBox);
    _mainLayout->addWidget(_encoderGroupBox);
    _mainLayout->addWidget(_controllerGroupBox);
    _mainLayout->addStretch();
}

void TrainingNodeWidget::applyStyles()
{
    StyleSheet styleSheet;
    QString customStyleSheet = styleSheet.mainStyleSheet;
    setStyleSheet(customStyleSheet);
}

void TrainingNodeWidget::updateConnectionStatus(const QString& status, const QColor& lightColor)
{
    if (_signalLight) {
        _signalLight->setColor(lightColor);
    }
    if (_statusLabel) {
        _statusLabel->setText(status);
    }
}

void TrainingNodeWidget::updateFromMessage(const QJsonObject& obj)
{
    // Process valid JSON object from training process and update UI components
    // Expects object with "status" field and optional training metrics (epoch, train_loss, etc.)
    
    // Handle different message types based on "status" field
    if (obj.contains("status")) {
        QString status = obj["status"].toString();
        
        if (status == "stage update") {
            if (obj.contains("stage")) {
                QString stage = obj["stage"].toString();
                updateConnectionStatus(stage, QColor(50, 255, 50)); // Bright green (like TRACER)
                qDebug() << "Stage update:" << stage;
            }
        }
        else if (status == "encoder training") {
            updateConnectionStatus("Training", QColor(50, 255, 50)); // Bright green (like TRACER)
            
            // Update progress if epoch information is available
            if (obj.contains("epoch")) {
                int epoch = obj["epoch"].toInt();
                if (_encoderProgressBar) {
                    _encoderProgressBar->updateValue(epoch);
                }
                qDebug() << "Training epoch:" << epoch;
            }
            
            // Update train loss if available
            if (obj.contains("train_loss")) {
                double trainLoss = obj["train_loss"].toDouble();
                if (_encoderTrainLossLabel) {
                    _encoderTrainLossLabel->setText(QString("Train Loss: %1").arg(trainLoss, 0, 'f', 4));
                }
                qDebug() << "Train loss:" << trainLoss;
            }
        }
        else if (status == "controller training") {
            updateConnectionStatus("Training", QColor(50, 255, 50)); // Bright green (like TRACER)
            
            // Update progress if epoch information is available
            if (obj.contains("epoch")) {
                int epoch = obj["epoch"].toInt();
                if (_controllerProgressBar) {
                    _controllerProgressBar->updateValue(epoch);
                }
                qDebug() << "Training epoch:" << epoch;
            }
            
            // Update train loss if available
            if (obj.contains("train_loss")) {
                double trainLoss = obj["train_loss"].toDouble();
                if (_controllerTrainLossLabel) {
                    _controllerTrainLossLabel->setText(QString("Train Loss: %1").arg(trainLoss, 0, 'f', 4));
                }
                qDebug() << "Train loss:" << trainLoss;
            }
        }
        else if (status == "completed") {
            updateConnectionStatus("Completed", QColor(50, 255, 50)); // Bright green (like TRACER)
            qDebug() << "Training completed successfully";
        }
        else if (status == "error" || status == "interrupted") {
            updateConnectionStatus("Error", Qt::red);
            if (obj.contains("message")) {
                qDebug() << "Training error:" << obj["message"].toString();
            }
        }else if (status == "verbose") {
            // Handle verbose messages
            if (obj.contains("message")) {
                qDebug() << "Training verbose:" << obj["message"].toString();
            }
        }
    }
}

void TrainingNodeWidget::resetProgress()
{
    if (_encoderProgressBar) {
        _encoderProgressBar->updateValue(0);
    }
}
