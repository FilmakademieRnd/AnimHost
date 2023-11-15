
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
    int numSamples = 12;
    int pastSamples = 6;
    int futureSamples = 5; // past samples + reference frame + future samples = numSamples

private:
    std::weak_ptr<AnimNodeData<Skeleton>> _skeletonIn;
    std::weak_ptr<AnimNodeData<PoseSequence>> _poseSequenceIn;
    std::weak_ptr<AnimNodeData<JointVelocitySequence>> _jointVelocitySequenceIn;
    std::weak_ptr<AnimNodeData<Animation>> _animationIn;

    QWidget* _widget = nullptr;
    BoneSelectionWidget* _boneSelect = nullptr;

private: 
    /* Input Data */

    std::vector<glm::vec2> posTrajectory; /*!< 2D Trajectory positions ground plane.*/
    std::vector<glm::vec2> forwardTrajectory /*!< 2D Trajectory of forward Vector. Hip orientation projected onto ground plane.*/;
    std::vector<glm::vec2> velTrajectory; /*!< 2D Trajectory of characters root velocities.*/
    std::vector<float> desSpeedTrajectory; /*!< 2D Trajectory of characters target root velocities.*/
    std::vector<char> oneHotActionType; /*!< One-hot encoded action types along trajectory.*/
    std::vector<glm::vec3> relativeJointPosition; /*!< Current joint positions relative to root position.*/
    std::vector<glm::quat> relativeJoitnRotations;
    std::vector<glm::vec3> relativeJointVelocities; 

    /* Output Data */

    std::vector<glm::vec2> predPosTrajectory;
    std::vector<glm::vec2> predForwardTrajectory;
    std::vector<glm::vec2> predVelTrajectory;
    std::vector<glm::vec3> predRelativeJointPosition;
    std::vector<glm::quat> predRelativJoitnRotations;
    std::vector<glm::vec3> predRelativeJointVelocities;

    glm::vec2 predRootTranslation;
    glm::vec2 predRootVelocity;
    float predRootAngularVelocity;

    

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

    void processRelativeRotations();

    QWidget* embeddedWidget() override;

private Q_SLOTS:
    void onRootBoneSelectionChanged(const QString& text);

};

#endif // MODEADAPTIVEPREPROCESSPLUGINPLUGIN_H
