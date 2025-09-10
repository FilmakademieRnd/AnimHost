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

class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;

class TRAININGPLUGINSHARED_EXPORT TrainingNodeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrainingNodeWidget(QWidget* parent = nullptr);
    ~TrainingNodeWidget() = default;

    void updateConnectionStatus(const QString& status, const QColor& lightColor);
    void updateFromMessage(const QJsonObject& obj);
    void resetProgress();

private:
    // UI components
    QVBoxLayout* _mainLayout;
    
    // ML Framework status components
    QGroupBox* _connectionGroupBox;
    QHBoxLayout* _connectionLayout;
    SignalLightWidget* _signalLight;
    QLabel* _statusLabel;
    
    // Training status components  
    QGroupBox* _trainingGroupBox;
    QVBoxLayout* _trainingLayout;
    ProgressWidget<int>* _progressWidget;
    QLabel* _trainLossLabel;
    
    void setupUI();
    void applyStyles();
};

#endif // TRAININGNODEWIDGET_H
