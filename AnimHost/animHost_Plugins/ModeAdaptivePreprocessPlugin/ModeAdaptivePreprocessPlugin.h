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

 

#ifndef MODEADAPTIVEPREPROCESSPLUGINPLUGIN_H
#define MODEADAPTIVEPREPROCESSPLUGINPLUGIN_H

#include "ModeAdaptivePreprocessPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>
#include <nodedatatypes.h>
#include <UIUtils.h>
#include <commondatatypes.h>
#include <MathUtils.h>


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

    //Define a forwardvector match forward of assimp
    glm::vec4 forwardBaseVector = glm::vec4(0, 0, 1.0, 0);
  


    int numSamples = 13;
    int pastSamples = 6;
    int futureSamples = 6; // past samples + reference frame + future samples = numSamples

    int rootbone_idx = 0;

	std::vector<glm::mat4> rootBoneTransforms; //<! The root bone transforms for each frame

	std::vector<std::vector<float>> rootSequenceData; //<! The root trajectory data for each frame, including the positional trajectory, direction, velocity, and speed relative to the root orientation, already flattened
	std::vector<std::vector<glm::vec3>> sequenceRelativeJointPosition; //<! The relative joint positions for each frame, relative to the reference frame
	std::vector<std::vector<glm::quat>> sequenceRelativJointRotations; //<! The relative joint rotations for each frame, relative to the reference frame
	std::vector<std::vector<Rotation6D>> sequenceRelativJointRotations6D; //<! The relative joint rotations for each frame, relative to the reference frame, converted to 6D rotations
	std::vector<std::vector<glm::vec3>> sequenceRelativeJointVelocities; //<! The relative joint velocities for each frame, relative to the reference frame

	std::vector<std::vector<float>> Y_RootSequenceData; //<! Output data: The root trajectory data for each frame, including the positional trajectory, direction, velocity, and speed relative to the root orientation, already flattened
	std::vector<std::vector<glm::vec3>> Y_SequenceRelativeJointPosition; //<! Output data: The relative joint positions for each frame, relative to the reference frame
	std::vector<std::vector<glm::quat>> Y_SequenceRelativJointRotations; //<! Output data: The relative joint rotations for each frame, relative to the reference frame
	std::vector<std::vector<Rotation6D>> Y_SequenceRelativJointRotations6D; //<! Output data: The relative joint rotations for each frame, relative to the reference frame, converted to 6D rotations
	std::vector<std::vector<glm::vec3>> Y_SequenceRelativeJointVelocities; //<! Output data: The relative joint velocities for each frame, relative to the reference frame
	std::vector<glm::vec3> Y_SequenceDeltaUpdate; //<! Output data: The delta update for the root position for each frame


public:
    ModeAdaptivePreprocessPlugin();
    ~ModeAdaptivePreprocessPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<ModeAdaptivePreprocessPlugin>(new ModeAdaptivePreprocessPlugin()); };

    QString category() override { return "Undefined Category"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    QJsonObject save() const override;
    void load(QJsonObject const& p) override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    
    bool isDataAvailable();
    
    void run() override;

    /**
     * This function processes a single frame of the animation sequence.
     * It calculates the root trajectory, joint positions, joint rotations, and joint velocities for the current frame and the next frame.
     * The calculated data is stored in the member variables of the class for later use.
     *
     * @param frameCounter The index of the frame to be processed.
     * @param poseSequenceIn A shared pointer to the PoseSequence that contains the joint positions for all frames.
     * @param animation A shared pointer to the Animation that contains the joint rotations for all frames.
     * @param velSeq A shared pointer to the JointVelocitySequence that contains the joint velocities for all frames.
     * @param skeleton A shared pointer to the Skeleton that contains the joint hierarchy.
     */
    void processFrame(int frameCounter, std::shared_ptr<PoseSequence> poseSequenceIn, std::shared_ptr<Animation> animation, std::shared_ptr<JointVelocitySequence> velSeq, std::shared_ptr<Skeleton> skeleton);



    /**
	 * Calculate character forward direction. Calculated the torsos forward direction by averaging the forward direction of the clavicle and hip joints.
     * 
     * \param poseSequenceIn
     * \param skeleton
     * \return 
     */
	std::vector<glm::quat> prepareRootRotation(std::shared_ptr<PoseSequence> poseSequenceIn, std::shared_ptr<Skeleton> skeleton);


    /**
	 * Calculate characters root tranform for whole sequence. 
	 * Rotation is based on averaged forward direction of clavicle and hip joints. 
	 * Position is based on the hip joint position projected to the ground plane.
     * 
     * \param poseSequenceIn
     * \param skeleton
     * \return 
     */
	std::vector<glm::mat4> prepareBipedRoot(std::shared_ptr<PoseSequence> poseSequenceIn, std::shared_ptr<Skeleton> skeleton);

    /**
     * This function prepares the trajectory data for a given frame.
     * It calculates the positional trajectory, direction, velocity, and speed relative to the root orientation.
     * This function is used in the processFrame function to calculate the trajectory data for the current frame and the next frame.
     *
     * @param referenceFrame The frame for which the trajectory data are to be calculated.
     * @param animation A shared pointer to the Animation that contains the joint rotations for all frames.
     * @param isOutput A boolean flag that indicates whether the function is being called for output data. If true, the function calculates the trajectory data for the next frame.
     * @return A pair consisting of a vector of floats representing the flattened trajectory data and a 2D vector representing the forward direction for the next frame.
     */
    std::vector<float> prepareTrajectoryData(int referenceFrame,  std::shared_ptr<Animation> animation, glm::mat4 Root, bool isOutput);

    /**
     * This function prepares the joint positions for a given frame.
     * It calculates the relative joint positions by subtracting the reference joint position from each joint position.
     * This function is used in the processFrame function to calculate the joint positions for the current frame and the next frame.
     *
     * @param referenceFrame The frame for which the joint positions are to be calculated.
     * @param poseSequenceIn A shared pointer to the PoseSequence that contains the joint positions for all frames.
     * @param isOutput A boolean flag that indicates whether the function is being called for output data. If true, the function calculates the joint positions for the next frame.
     * @return A vector of 3D vectors representing the relative joint positions for the given frame.
     */
    std::vector<glm::vec3> prepareJointPositions(int referenceFrame, std::shared_ptr<PoseSequence> poseSequenceIn, glm::mat4 Root, bool isOutput = false);

    /**
     * This function prepares the joint rotations for a given frame.
     * It calculates the relative joint rotations by multiplying the joint rotation with the inverse of the reference joint rotation.
     * This function is used in the processFrame function to calculate the joint rotations for the current frame and the next frame.
     *
     * @param referenceFrame The frame for which the joint rotations are to be calculated.
     * @param animation A shared pointer to the Animation that contains the joint rotations for all frames.
     * @param skeleton A shared pointer to the Skeleton that contains the joint hierarchy.
     * @param inverseReferenceJointRotation The inverse of the reference joint rotation.
     * @param isOutput A boolean flag that indicates whether the function is being called for output data. If true, the function calculates the joint rotations for the next frame.
     * @return A vector of quaternions representing the relative joint rotations for the given frame.
     */
    std::vector<glm::quat> prepareJointRotations(int referenceFrame, std::shared_ptr<Animation> animation, std::shared_ptr<Skeleton> skeleton, glm::mat4 Root, bool isOutput = false);
    std::vector<Rotation6D> prepareJointRotations6D(int referenceFrame, std::shared_ptr<Animation> animation, std::shared_ptr<Skeleton> skeleton, glm::mat4 Root, bool isOutput);


    /**
     * This function prepares the joint velocities for a given frame.
     * It calculates the relative joint velocities by multiplying the joint velocity with the inverse of the reference joint rotation.
     * This function is used in the processFrame function to calculate the joint velocities for the current frame and the next frame.
     *
     * @param referenceFrame The frame for which the joint velocities are to be calculated.
     * @param velSeq A shared pointer to the JointVelocitySequence that contains the joint velocities for all frames.
     * @param inverseReferenceJointRotation The inverse of the reference joint rotation.
     * @param isOutput A boolean flag that indicates whether the function is being called for output data. If true, the function calculates the joint velocities for the next frame.
     * @return A vector of 3D vectors representing the relative joint velocities for the given frame.
     */
    std::vector<glm::vec3> prepareJointVelocities(int referenceFrame, std::shared_ptr<JointVelocitySequence> velSeq,  glm::mat4 Root, bool isOutput = false);

    QWidget* embeddedWidget() override;

    /** Experimental */
    std::vector<glm::quat> ApplyGaussianFilter(const std::vector<glm::quat>& rotations, const std::vector<float>& weights, float sigma);
    std::vector<float> ComputeAdaptiveWeights(const std::vector<glm::quat>& rotations, float sigma);
    std::vector<glm::quat> SmoothRootRotations(const std::vector<glm::quat>& rootRotations, float timeWindow);

    std::vector<glm::quat> GaussianFilterQuaternions(const std::vector<glm::quat>& quaternions, float sigma);


private:
    void clearExistingData();
    void writeInputData();
    void writeOutputData();
    void writeMetaData();

private Q_SLOTS:
    void onRootBoneSelectionChanged(const int text);
    void onFolderSelectionChanged();
    void onOverrideCheckbox(int state);
    
};

#endif // MODEADAPTIVEPREPROCESSPLUGINPLUGIN_H
