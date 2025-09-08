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


#ifndef TRAINFRAMEWORKNODE_H
#define TRAINFRAMEWORKNODE_H

#include "../TrainFrameworkPlugin_global.h"
#include <QMetaType>
#include <QtWidgets>
#include <QTimer>
#include <pluginnodeinterface.h>
#include <nodedatatypes.h>
#include <UIUtils.h>

class QPushButton;

class TRAINFRAMEWORKPLUGINSHARED_EXPORT TrainFrameworkNode : public PluginNodeInterface
{
    Q_OBJECT

private:
    QWidget* _widget;
    ProgressWidget<int>* _progressWidget;
    QTimer* _timer;
    int _currentStep;
    
    // Framework automatically provides RunSignal input at port 0

public:
    TrainFrameworkNode();
    ~TrainFrameworkNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return std::unique_ptr<TrainFrameworkNode>(new TrainFrameworkNode()); };

    static QString Name() { return QString("TrainFrameworkNode"); }

    QString category() override { return "Examples"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    bool isDataAvailable() override;

    void run() override;

    QWidget* embeddedWidget() override;

private Q_SLOTS:
    void onTimerTimeout();

};

#endif // TRAINFRAMEWORKNODE_H