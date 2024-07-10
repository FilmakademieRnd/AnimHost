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

 

#ifndef RUNTRIGGERPLUGINPLUGIN_H
#define RUNTRIGGERPLUGINPLUGIN_H

#include "RunTriggerPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>

class QPushButton;

class RUNTRIGGERPLUGINSHARED_EXPORT RunTriggerPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.RunTrigger" FILE "RunTriggerPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    QPushButton* _pushButton;

public:
    RunTriggerPlugin();
    ~RunTriggerPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<RunTriggerPlugin>(new RunTriggerPlugin()); };

    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    bool hasInputRunSignal() const override { return false; };

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;

    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    bool isDataAvailable() override { return true; };
    void run() override {};

    QWidget* embeddedWidget() override;

    //QTNodes
    QString category() override { return "Signal"; };  // Returns a category for the node

private Q_SLOTS:
    void onButtonClicked();

};

#endif // RUNTRIGGERPLUGINPLUGIN_H
