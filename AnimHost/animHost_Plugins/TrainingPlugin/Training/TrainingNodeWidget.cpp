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
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMessageBox>

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
    , _artifactsGroupBox(nullptr)
    , _viewArtifactsButton(nullptr)
    , _runDir("")
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
    
    _encoderProgressBar = new ProgressWidget<int>("Epoch", 1, _encoderGroupBox);
    _encoderTrainLossLabel = new QLabel("Training Loss: --", _encoderGroupBox);
    
    trainingLayout->addWidget(_encoderProgressBar);
    trainingLayout->addWidget(_encoderTrainLossLabel);

    // Create Controller Status Group Box
    _controllerGroupBox = new QGroupBox("Controller Status", this);
    auto* controllerLayout = new QVBoxLayout(_controllerGroupBox);

    _controllerProgressBar = new ProgressWidget<int>("Epoch", 1, _controllerGroupBox);
    _controllerTrainLossLabel = new QLabel("Training Loss: --", _controllerGroupBox);

    controllerLayout->addWidget(_controllerProgressBar);
    controllerLayout->addWidget(_controllerTrainLossLabel);

    // Create Artifacts Group Box
    _artifactsGroupBox = new QGroupBox("Artifacts", this);
    auto* artifactsLayout = new QHBoxLayout(_artifactsGroupBox);

    _viewArtifactsButton = new QPushButton("View Run Artifacts 📁", _artifactsGroupBox);
    _viewArtifactsButton->setEnabled(false);
    _viewArtifactsButton->setToolTip("No run directory configured");

    connect(_viewArtifactsButton, &QPushButton::clicked, this, &TrainingNodeWidget::onViewArtifactsClicked);

    artifactsLayout->addWidget(_viewArtifactsButton);

    // Add group boxes to main layout
    _mainLayout->addWidget(_connectionGroupBox);
    _mainLayout->addWidget(_encoderGroupBox);
    _mainLayout->addWidget(_controllerGroupBox);
    _mainLayout->addWidget(_artifactsGroupBox);
    _mainLayout->addStretch();
}

void TrainingNodeWidget::applyStyles()
{
    StyleSheet styleSheet;
    QString customStyleSheet = styleSheet.mainStyleSheet;
    setStyleSheet(customStyleSheet);
}

void TrainingNodeWidget::updateConnectionStatus(const QString& status, const QColor& signalColor, const QString& statusText)
{
    if (_signalLight) {
        _signalLight->setColor(signalColor);
    }
    if (_statusLabel) {
        _statusLabel->setText(status);
        _statusLabel->setToolTip(statusText);
    }
}

void TrainingNodeWidget::updateFromMessage(const MLFramework::TrainingMessage& msg)
{
    // Default status display with green light (can be overridden)
    updateConnectionStatus(msg.status, QColor(50, 255, 50), msg.text);

    if (msg.status == "Encoder training") {
        // Update progress if epoch information is available
        if (msg.metrics.contains("epoch")) {
            int epoch = msg.metrics["epoch"].toInt();
            if (_encoderProgressBar) {
                _encoderProgressBar->updateValue(epoch);
            }
            qDebug() << "Training epoch:" << epoch;
        }
        
        // Update train loss if available
        if (msg.metrics.contains("train_loss")) {
            double trainLoss = msg.metrics["train_loss"].toDouble();
            if (_encoderTrainLossLabel) {
                _encoderTrainLossLabel->setText(QString("Train Loss: %1").arg(trainLoss, 0, 'f', 4));
            }
            qDebug() << "Train loss:" << trainLoss;
        }
    }
    else if (msg.status == "Controller training") {
        // Update progress if epoch information is available
        if (msg.metrics.contains("epoch")) {
            int epoch = msg.metrics["epoch"].toInt();
            if (_controllerProgressBar) {
                _controllerProgressBar->updateValue(epoch);
            }
            qDebug() << "Training epoch:" << epoch;
        }
        
        // Update train loss if available
        if (msg.metrics.contains("train_loss")) {
            double trainLoss = msg.metrics["train_loss"].toDouble();
            if (_controllerTrainLossLabel) {
                _controllerTrainLossLabel->setText(QString("Train Loss: %1").arg(trainLoss, 0, 'f', 4));
            }
            qDebug() << "Train loss:" << trainLoss;
        }
    }
    else if (msg.status == "Error") {
        updateConnectionStatus("Error", Qt::red);
        if (!msg.text.isEmpty()) {
            qDebug() << "Training error:" << msg.text;
        }
    }
}

void TrainingNodeWidget::resetProgress()
{
    if (_encoderProgressBar) {
        _encoderProgressBar->updateValue(0);
    }
    if (_controllerProgressBar) {
        _controllerProgressBar->updateValue(0);
    }
}

void TrainingNodeWidget::setConfiguration(const MLFramework::StarkeConfig& config)
{
    if (_encoderProgressBar) {
        _encoderProgressBar->setMaxValue(config.pae_epochs);
    }
    if (_controllerProgressBar) {
        _controllerProgressBar->setMaxValue(config.gnn_epochs);
    }
}

void TrainingNodeWidget::setRunDir(const QString& runDir)
{
    _runDir = runDir;

    // Update button tooltip based on whether run_dir is set
    if (_viewArtifactsButton) {
        if (_runDir.isEmpty()) {
            _viewArtifactsButton->setToolTip("No run directory configured");
        } else {
            _viewArtifactsButton->setToolTip(_runDir);
        }
    }
}

void TrainingNodeWidget::setArtifactsButtonEnabled(bool enabled)
{
    if (!_viewArtifactsButton) {
        return;
    }

    // Check if directory exists before enabling
    if (enabled && !_runDir.isEmpty()) {
        QDir dir(_runDir);
        if (dir.exists()) {
            _viewArtifactsButton->setEnabled(true);
            _viewArtifactsButton->setToolTip(_runDir);
        } else {
            _viewArtifactsButton->setEnabled(false);
            _viewArtifactsButton->setToolTip("Artifacts directory does not exist: " + _runDir);
        }
    } else if (enabled && _runDir.isEmpty()) {
        _viewArtifactsButton->setEnabled(false);
        _viewArtifactsButton->setToolTip("No run directory configured");
    } else {
        _viewArtifactsButton->setEnabled(false);
        if (_runDir.isEmpty()) {
            _viewArtifactsButton->setToolTip("No run directory configured");
        } else {
            _viewArtifactsButton->setToolTip("Training in progress...");
        }
    }
}

void TrainingNodeWidget::onViewArtifactsClicked()
{
    if (_runDir.isEmpty()) {
        QMessageBox::warning(this, "No Artifacts", "No run directory has been configured.");
        return;
    }

    QDir dir(_runDir);
    if (!dir.exists()) {
        QMessageBox::warning(this, "Directory Not Found",
                            "The artifacts directory no longer exists:\n" + _runDir);
        return;
    }

    // Open directory in Windows Explorer using QDesktopServices
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(_runDir))) {
        QMessageBox::warning(this, "Failed to Open",
                            "Failed to open artifacts directory:\n" + _runDir);
    }
}
