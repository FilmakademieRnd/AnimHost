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
    , _trainingGroupBox(nullptr)
    , _trainingLayout(nullptr)
    , _progressWidget(nullptr)
    , _trainLossLabel(nullptr)
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
    
    // Create Training Status Group Box
    _trainingGroupBox = new QGroupBox("Training Status", this);
    _trainingLayout = new QVBoxLayout(_trainingGroupBox);
    
    _progressWidget = new ProgressWidget<int>("Epoch", 5, _trainingGroupBox);
    _trainLossLabel = new QLabel("Train Loss: --", _trainingGroupBox);
    
    _trainingLayout->addWidget(_progressWidget);
    _trainingLayout->addWidget(_trainLossLabel);
    
    // Add group boxes to main layout
    _mainLayout->addWidget(_connectionGroupBox);
    _mainLayout->addWidget(_trainingGroupBox);
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
        
        if (status == "starting") {
            updateConnectionStatus("Starting", QColor(255, 200, 50)); // Orange (like TRACER)
        }
        else if (status == "training") {
            updateConnectionStatus("Training", QColor(50, 255, 50)); // Bright green (like TRACER)
            
            // Update progress if epoch information is available
            if (obj.contains("epoch")) {
                int epoch = obj["epoch"].toInt();
                if (_progressWidget) {
                    _progressWidget->updateValue(epoch);
                }
                qDebug() << "Training epoch:" << epoch;
            }
            
            // Update train loss if available
            if (obj.contains("train_loss")) {
                double trainLoss = obj["train_loss"].toDouble();
                if (_trainLossLabel) {
                    _trainLossLabel->setText(QString("Train Loss: %1").arg(trainLoss, 0, 'f', 4));
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
        }
    }
}

void TrainingNodeWidget::resetProgress()
{
    if (_progressWidget) {
        _progressWidget->updateValue(0);
    }
}