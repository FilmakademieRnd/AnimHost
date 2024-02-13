
#ifndef MODEADAPTIVEPREPROCESSPLUGINPLUGIN_H
#define MODEADAPTIVEPREPROCESSPLUGINPLUGIN_H

#include "ModeAdaptivePreprocessPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>
#include <nodedatatypes.h>
#include <UIUtils.h>
#include <commondatatypes.h>


class MODEADAPTIVEPREPROCESSPLUGINSHARED_EXPORT ModeAdaptivePreprocessPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.ModeAdaptivePreprocess" FILE "ModeAdaptivePreprocessPlugin.json")
    Q_INTERFACES(PluginNodeInterface)


private:
    std::weak_ptr<AnimNodeData<Skeleton>> _skeletonIn;
    std::weak_ptr<AnimNodeData<PoseSequence>> _poseSequenceIn;
    std::weak_ptr<AnimNodeData<JointVelocitySequence>> _jointVelocitySequenceIn;
    std::weak_ptr<AnimNodeData<Animation>> _animationIn;


    //UI
    QWidget* _widget = nullptr;
    FolderSelectionWidget* _folderSelect = nullptr;
    BoneSelectionWidget* _boneSelect = nullptr;
    QCheckBox* _cbOverwrite = nullptr;
    QVBoxLayout* _vLayout = nullptr;


private: 
    //Write Data
    QString exportDirectory;

    int totalNumberFrames = 0;
    bool bOverwriteDataExport = false;

    QString metadataFileName = "metadata.txt";
    QString sequencesFileName = "sequences_mann.txt";
    QString dataXFileName = "data_X.bin";
    QString dataYFileName = "data_Y.bin";


    int numSamples = 12;
    int pastSamples = 6;
    int futureSamples = 5; // past samples + reference frame + future samples = numSamples

    int rootbone_idx = 0;

    std::vector<std::vector<float>> rootSequenceData;
    std::vector<std::vector<glm::vec3>> sequenceRelativeJointPosition;
    std::vector<std::vector<glm::quat>> sequenceRelativJointRotations;
    std::vector<std::vector<glm::vec3>> sequenceRelativeJointVelocities;

    std::vector<std::vector<float>> Y_RootSequenceData;
    std::vector<std::vector<glm::vec3>> Y_SequenceRelativeJointPosition;
    std::vector<std::vector<glm::quat>> Y_SequenceRelativJointRotations;
    std::vector<std::vector<glm::vec3>> Y_SequenceRelativeJointVelocities;
    std::vector<glm::vec3> Y_SequenceDeltaUpdate;


public:
    ModeAdaptivePreprocessPlugin();
    ~ModeAdaptivePreprocessPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<ModeAdaptivePreprocessPlugin>(new ModeAdaptivePreprocessPlugin()); };

    QString category() override { return "Undefined Category"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    void run() override;

    QWidget* embeddedWidget() override;


private:
    void writeInputData();
    void writeOutputData();
    void writeMetaData();

private Q_SLOTS:
    void onRootBoneSelectionChanged(const int text);
    void onFolderSelectionChanged();
    void onOverrideCheckbox(int state);

};

#endif // MODEADAPTIVEPREPROCESSPLUGINPLUGIN_H
