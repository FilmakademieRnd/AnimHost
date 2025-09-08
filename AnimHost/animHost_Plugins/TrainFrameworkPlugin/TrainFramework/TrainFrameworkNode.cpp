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

#include "TrainFrameworkNode.h"
#include <QPushButton>

TrainFrameworkNode::TrainFrameworkNode()
{
    _widget = nullptr;
    _progressWidget = nullptr;
    _timer = new QTimer(this);
    _currentStep = 0;
    
    _timer->setSingleShot(true);
    connect(_timer, &QTimer::timeout, this, &TrainFrameworkNode::onTimerTimeout);
    
    qDebug() << "TrainFrameworkNode created";
}

TrainFrameworkNode::~TrainFrameworkNode()
{
    qDebug() << "~TrainFrameworkNode()";
}

unsigned int TrainFrameworkNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;  // No custom input ports
    else
        return 0;  // No output ports
}

NodeDataType TrainFrameworkNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    return NodeDataType{};
}

void TrainFrameworkNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    // No custom input ports - framework handles RunSignal automatically
    // This method won't be called since nDataPorts returns 0 for input
    qDebug() << "TrainFrameworkNode processInData called unexpectedly";
}

std::shared_ptr<NodeData> TrainFrameworkNode::processOutData(QtNodes::PortIndex port)
{
    return nullptr;  // No output data
}

bool TrainFrameworkNode::isDataAvailable() 
{
    // We can always run - either from button click or run signal
    return true;
}

void TrainFrameworkNode::run()
{
    qDebug() << "TrainFrameworkNode run - Training Framework!";
    
    // Reset and start the training simulation
    _currentStep = 0;
    if (_progressWidget) {
        _progressWidget->updateValue(0);
    }
    
    // Start the first step after 500ms
    _timer->start(500);
}

QWidget* TrainFrameworkNode::embeddedWidget()
{
    if (!_widget) {
        _widget = new QWidget();
        _progressWidget = new ProgressWidget<int>("Epoch", 3, _widget);
        
        auto layout = new QVBoxLayout(_widget);
        layout->addWidget(_progressWidget);
        
        StyleSheet styleSheet;
        QString customStyleSheet = styleSheet.mainStyleSheet + 
            "QProgressBar{"
            "border: 1px solid white;"
            "border-radius: 4px;"
            "background-color: rgb(25, 25, 25);"
            "text-align: center;"
            "color: white;"
            "}"
            "QProgressBar::chunk{"
            "background-color: rgb(98, 139, 202);"
            "border-radius: 3px;"
            "}";
        
        _widget->setStyleSheet(customStyleSheet);
    }
    
    return _widget;
}

void TrainFrameworkNode::onTimerTimeout()
{
    _currentStep++;
    
    if (_progressWidget && _progressWidget->updateValue(_currentStep)) {
        qDebug() << "Training step" << _currentStep << "completed";
        
        if (_currentStep < 3) {
            // Continue to next step after 500ms
            _timer->start(500);
        } else {
            qDebug() << "Training completed!";
        }
    }
}