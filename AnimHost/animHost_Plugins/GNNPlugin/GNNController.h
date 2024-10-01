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

 
#include "GNNPlugin_global.h"
#include "HistoryBuffer.h"
#include "OnnxModel.h"
#include "PhaseSequence.h"
#include "RootSeries.h"

//#include <matplot/matplot.h>


/**
 * @struct JointsFrameData
 * @brief A structure to hold frame data for multiple joints.
 *
 * This structure encapsulates the position, rotation, and velocity of multiple joints in a frame.
 * Each member is a vector that represents a different aspect of the joints' state.
 *
 * @var std::vector<glm::vec3> jointPos
 *   A vector of 3D vectors representing the positions of the joints.
 *
 * @var std::vector<glm::quat> jointRot
 *   A vector of quaternions representing the rotations of the joints.
 *
 * @var std::vector<glm::vec3> jointVel
 *   A vector of 3D vectors representing the velocities of the joints.
 */
struct JointsFrameData
{
    std::vector<glm::vec3> jointPos;
    std::vector<glm::quat> jointRot;
    std::vector<glm::vec3> jointVel;

    void clear()
    {
		jointPos.clear();
		jointRot.clear();
		jointVel.clear();
	}
};

struct TrajectoryFrameData
{
	std::vector<glm::vec2> pos;
	std::vector<glm::vec2> dir;
	std::vector<glm::vec2> vel;
	std::vector<float> speed;

    void clear()
    {
		pos.clear();
		dir.clear();
		vel.clear();
		speed.clear();
	}
};

class GNNPLUGINSHARED_EXPORT GNNController
{
private:

    int pastKeys = 6;
    int futureKeys = 6;

    int totalKeys = pastKeys + futureKeys + 1;

    int numPhaseChannel = 5;

    /*Mix Weights*/
    float rootTranslationWeight = 0.5f;
    float rootRotationWeight = 0.5f;
    float tau = 1.f;

    /* Control Trajectory derived from controll signal(offline process) */
    
    //desired positional trajectory of character
    std::vector<glm::vec2> ctrlTrajPos;

    //desired forward faceing direction
    std::vector<glm::quat> ctrlTrajForward;

    //desired velocity of character
    std::vector<glm::vec2> ctrlTrajVel;
   
    /* Phase data */
    PhaseSequence phaseSequence;
  
    //generated positional trajectory of character
    std::vector<glm::vec2> genRootPos;

    //generated forward facing direction
    std::vector<glm::quat> genRootForward;

    //generated joint positions
    std::vector<std::vector<glm::vec3>> genJointPos;

    //generated joint rotations
    std::vector<std::vector<glm::quat>> genJointRot;

    //generated joint velocity
    std::vector<std::vector<glm::vec3>> genJointVel;

    std::vector<std::vector<glm::vec2>> genPhase2D;

    std::shared_ptr<Skeleton> skeleton;

    std::shared_ptr<ControlPath> controlPath;

    std::shared_ptr<Animation> animationIn;

    std::shared_ptr<Animation> animationOut;

    std::shared_ptr<DebugSignal> debugSignal;

    //in & output tensors

    std::vector<float> input_values;
    std::vector<float> output_values;

    //Plotting
    //matplot::figure_handle figure = nullptr;


public:

    /* Neural Network */
    std::unique_ptr<OnnxModel> network;
    QString NetworkModelPath = "C:\\DEV\\AI4Animation\\AI4Animation\\SIGGRAPH_2022\\PyTorch\\GNN\\Training\\149.onnx";

public:
    //initial joint positions
    std::vector<glm::vec3> initJointPos;

    //initial joint rotations
    std::vector<glm::quat> initJointRot;

    //initial jont velocity
    std::vector<glm::vec3> initJointVel;

public:

    GNNController(QString networkPath);
    
    void prepareInput();

    void SetSkeleton(std::shared_ptr<Skeleton> skel);

    void SetAnimationIn(std::shared_ptr<Animation> anim);

    void SetControlPath(std::shared_ptr<ControlPath> path);

    void SetMixWeights(float translation, float rotation, float controlTau) {
        rootTranslationWeight = translation; rootRotationWeight = rotation; tau = controlTau; }

    std::shared_ptr<Animation> GetAnimationOut();
    std::shared_ptr<DebugSignal> GetDebugSignal(){return debugSignal; }

private:

    void InitPlot();
    void UpdatePlotData(const TrajectoryFrameData& inTrajFrame, const TrajectoryFrameData& outTrajFrame, const RootSeries& rootSeries, const std::vector<glm::vec2>& futurePath);
    void DrawPlot();


    void clearGeneratedData();
    void prepareControlTrajectory();

    void BuildAnimationSequence(const std::vector<std::vector<glm::quat>>& jointRotSequence, const RootSeries& rootSeries);

    TrajectoryFrameData BuildTrajectoryFrameData(const RootSeries& rootSeries, glm::mat4 Root);

    
    void BuildInputTensor(const TrajectoryFrameData& inTrajFrame,
        const JointsFrameData& inJointFrame);
    
    glm::vec3  readOutput(const std::vector<float>& output_values, TrajectoryFrameData& outTrajectoryFrame, JointsFrameData& outJointFrame,
        std::vector<std::vector<glm::vec2>>& outPhase2D, std::vector<std::vector<float>>& outAmplitude, 
        std::vector<std::vector<float>>& outFrequency);

    std::vector<glm::quat> ConvertRotationsToLocalSpace(const std::vector<glm::quat>& relativeJointRots);

    glm::vec2 Calc2DPhase(float phaseValue, float amplitude);

    glm::vec2 Update2DPhase(float amplitude, float frequency, glm::vec2 current, glm::vec2 next, float minAmplitude);

    float CalcPhaseValue(glm::vec2 phase);

    glm::mat4 updateRootTranform(const glm::mat4& pos, const glm::vec3& delta, int genIdx);

};