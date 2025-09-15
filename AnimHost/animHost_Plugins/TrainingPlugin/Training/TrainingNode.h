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


#ifndef TRAININGNODE_H
#define TRAININGNODE_H

#include "../TrainingPlugin_global.h"
#include <QMetaType>
#include <QtWidgets>
#include <QTimer>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <pluginnodeinterface.h>
#include <nodedatatypes.h>

class TrainingNodeWidget;

class TRAININGPLUGINSHARED_EXPORT TrainingNode : public PluginNodeInterface
{
    Q_OBJECT

private:
    // UI
    TrainingNodeWidget* _widget;
    
    // Process management
    QProcess* _trainingProcess;
    QString _pythonScriptPath;
    
    // Framework automatically provides RunSignal input at port 0

public:
    TrainingNode();
    ~TrainingNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return std::unique_ptr<TrainingNode>(new TrainingNode()); };

    static QString Name() { return QString("TrainingNode"); }

    QString category() override { return "MLFramework"; };
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
    void onTrainingOutput();
    void onTrainingFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTrainingError();
    void onTrainingProcessError(QProcess::ProcessError error);

private:
    void updateConnectionStatus(const QString& status, const QColor& lightColor);
    void updateFromMessage(const QJsonObject& obj);

};

#endif // TRAININGNODE_H
