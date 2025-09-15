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

#ifndef TRAININGNODEWIDGET_H
#define TRAININGNODEWIDGET_H

#include "../TrainingPlugin_global.h"
#include <QtWidgets>
#include <QJsonObject>
#include <UIUtils.h>
#include "MLFrameworkTypes.h"

class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;

class TRAININGPLUGINSHARED_EXPORT TrainingNodeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrainingNodeWidget(QWidget* parent = nullptr);
    ~TrainingNodeWidget() = default;

    /**
     * Update the connection status display.
     * @param status The status text to display.
     * @param lightColor The color of the status light.
     * @param statusMessage An optional detailed status message.
     */
    void updateConnectionStatus(const QString& status, const QColor& lightColor, const QString& statusText = "");
    /**
     * Update the widget based on a training message.
     * @param msg The training message containing status and metrics.
     */
    void updateFromMessage(const MLFramework::TrainingMessage& msg);
    /**
     * Reset progress bars to initial state.
     */
    void resetProgress();

private:
    // UI components
    QVBoxLayout* _mainLayout;
    
    // ML Framework status components
    QGroupBox* _connectionGroupBox;
    QHBoxLayout* _connectionLayout;
    SignalLightWidget* _signalLight;
    QLabel* _statusLabel;
    
    // Encoder status components  
    QGroupBox* _encoderGroupBox;
    ProgressWidget<int>* _encoderProgressBar;
    QLabel* _encoderTrainLossLabel;

    // Controller status components
    QGroupBox* _controllerGroupBox;
    ProgressWidget<int>* _controllerProgressBar;
    QLabel* _controllerTrainLossLabel;

    void setupUI();
    void applyStyles();
};

#endif // TRAININGNODEWIDGET_H
