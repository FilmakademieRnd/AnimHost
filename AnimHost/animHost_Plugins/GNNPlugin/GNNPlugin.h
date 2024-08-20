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

 

#ifndef GNNPLUGINPLUGIN_H
#define GNNPLUGINPLUGIN_H

#include "GNNPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>
#include "GNNController.h"
#include "UIUtils.h"




class QPushButton;

class GNNPLUGINSHARED_EXPORT GNNPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.GNN" FILE "GNNPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:

    // Input Data
    std::weak_ptr<AnimNodeData<Animation>> _animationIn;
    std::weak_ptr<AnimNodeData<Skeleton>> _skeletonIn;
    std::weak_ptr<AnimNodeData<ControlPath>> _controlPathIn;
    std::weak_ptr<AnimNodeData<JointVelocitySequence>> _jointVelocitySequenceIn;


    //Output Data
    std::shared_ptr<AnimNodeData<Animation>> _animationOut;
    std::shared_ptr<AnimNodeData<DebugSignal>> _debugSignalOut;

    //Neural Network Controller
    std::unique_ptr<GNNController> controller;
    QString _NetworkPath;

    //UI
    QWidget* _widget = nullptr;
    FolderSelectionWidget* _fileSelectionWidget = nullptr;
    QDoubleSpinBox* _mixRootRotation = nullptr;
    QDoubleSpinBox* _mixRootTranslation = nullptr;
    QDoubleSpinBox* _mixControlPath = nullptr;

   

public:
    GNNPlugin();
    GNNPlugin(bool active) {
        _widget = nullptr;
        qDebug() << "ACTIVE GNNPlugin created";
    };
    ~GNNPlugin();

    QJsonObject save() const override;
    void load(QJsonObject const& p) override;
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<GNNPlugin>(new GNNPlugin(true)); };


    QString category() override { return "Undefined Category"; };
    QString caption() const override { return "Locomotion Generator (2D Spline)"; }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    bool isDataAvailable();
    void run() override;

    QWidget* embeddedWidget() override;

private Q_SLOTS:
    void onFileSelectionChanged();
};

#endif // GNNPLUGINPLUGIN_H
