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

 

#ifndef DATAEXPORTPLUGINPLUGIN_H
#define DATAEXPORTPLUGINPLUGIN_H

#include "DataExportPlugin_global.h"
#include <QMetaType>
#include <QtWidgets>
#include <pluginnodeinterface.h>
#include <nodedatatypes.h>

class QPushButton;

class DATAEXPORTPLUGINSHARED_EXPORT DataExportPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.DataExport" FILE "DataExportPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:

    QWidget* widget = nullptr;
    QPushButton* _pushButton = nullptr;
    QLabel* _label = nullptr;
    QHBoxLayout* _filePathLayout = nullptr;
    QCheckBox* _cbWriteBinary = nullptr;
    QCheckBox* _cbOverwrite = nullptr;
    QVBoxLayout* _vLayout = nullptr;

    // Node Data Innput
    std::weak_ptr<AnimNodeData<Skeleton>> _skeletonIn;

    bool bWritePoseSequence = true;
    std::weak_ptr<AnimNodeData<PoseSequence>> _poseSequenceIn;
    bool bWriteJointVelocity = true;
    std::weak_ptr<AnimNodeData<JointVelocitySequence>> _jointVelocitySequenceIn;

    // Export Settings
    QString exportDirectory = "";

    bool bWriteBinaryData = false;
    bool bOverwriteJointVelSeq = true;
    bool bOverwritePoseSeq = true;

public:

    DataExportPlugin();
    ~DataExportPlugin();

    QJsonObject save() const override;

    void load(QJsonObject const& p) override;
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<DataExportPlugin>(new DataExportPlugin()); };

    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    bool isDataAvailable() override;
    void run() override;

    QWidget* embeddedWidget() override;

    void exportPoseSequenceData();
    void writeCSVPoseSequenceData();
    void writeBinaryPoseSequenceData();

    void writeBinarySkeletonData();

    void exportJointVelocitySequence();
    void writeCSVJointVelocitySequence();
    void writeBinaryJointVelocitySequence();

    //QTNodes
    QString category() override { return "Undefined Category"; };  // Returns a category for the node

private Q_SLOTS:
    void onButtonClicked();
    void onOverrideCheckbox(int state);

};

#endif // DATAEXPORTPLUGINPLUGIN_H
