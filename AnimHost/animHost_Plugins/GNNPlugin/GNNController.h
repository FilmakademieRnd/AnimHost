#include "GNNPlugin_global.h"
#include "HistoryBuffer.h"
#include "OnnxModel.h"
#include "PhaseSequence.h"


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
};

struct TrajectoryFrameData
{
	std::vector<glm::vec2> pos;
	std::vector<glm::vec2> dir;
	std::vector<glm::vec2> vel;
	std::vector<float> speed;
};

class GNNPLUGINSHARED_EXPORT GNNController
{
private:

    int pastKeys = 6;
    int futureKeys = 6;

    int totalKeys = pastKeys + futureKeys + 1;

    int numPhaseChannel = 5;

    /* Control Trajectory derived from controll signal(offline process) */
    
    //desired positional trajectory of character
    std::vector<glm::vec2> ctrlTrajPos;

    //desired forward faceing direction
    std::vector<glm::quat> ctrlTrajForward;

    //desired velocity of character
    std::vector<glm::vec2> ctrlTrajVel;

    /* Initial joint values defining start pose of character animation.
       Predefined for testing and prototype. */

   

    /* Phase data */
    //std::vector<float> Phase;
    //std::vector<float> Amplitude;

    //std::vector<glm::vec2> phase2D;

    PhaseSequence phaseSequence;

    /* Generated  deserialized network output.
       Cleanup and blending also computet on output. */
  
    //generated positional trajectory of character
    std::vector<glm::vec2> genTrajPos;

    //generated forward facing direction
    std::vector<glm::vec2> genTrajForward;

    //generated velocity of character
    std::vector<glm::vec2> genTrajVel;

    //generated joint positions
    std::vector<std::vector<glm::vec3>> genJointPos;

    //generated joint rotations
    std::vector<std::vector<glm::quat>> genJointRot;

    //generated joint velocity
    std::vector<std::vector<glm::vec3>> genJointVel;


    std::vector<std::vector<glm::vec2>> genPhase2D;


    //Skeleton
    std::shared_ptr<Skeleton> skeleton;

    std::shared_ptr<ControlPath> controlPath;

    std::shared_ptr<Animation> animationIn;

    std::shared_ptr<Animation> animationOut;

    std::shared_ptr<DebugSignal> debugSignal;




    //normalisation

    std::vector<float> stdIn;
    std::vector<float> meanIn;

    std::vector<float> stdOut;
    std::vector<float> meanOut;

    //in & output tensors
    std::vector<float> dummyIn;
    std::vector<float> dummyPhase;
    std::vector<float> input_values;
    //std::vector<float> output_values;

    int currentPivot = 0;

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

    void InitDummyData();

    void SetSkeleton(std::shared_ptr<Skeleton> skel);

    void SetAnimationIn(std::shared_ptr<Animation> anim);

    void SetControlPath(std::shared_ptr<ControlPath> path);

    std::shared_ptr<Animation> GetAnimationOut();
    std::shared_ptr<DebugSignal> GetDebugSignal(){return debugSignal; }

    void BuildAnimationSequence(const std::vector<std::vector<glm::quat>>& jointRotSequence);
    
 

private:

    TrajectoryFrameData BuildTrajectoryFrameData(const std::vector<glm::vec2>controlTrajectoryPositions, const std::vector<glm::quat>& controlTrajectoryForward, 
        const TrajectoryFrameData& inferredTrajectoryFrame, int PivotFrame, glm::mat4 Root);
    

    void BuildInputTensor(const TrajectoryFrameData& inTrajFrame,
        const JointsFrameData& inJointFrame);
    
    glm::vec3  readOutput(const std::vector<float>& output_values, TrajectoryFrameData& outTrajectoryFrame, JointsFrameData& outJointFrame,
        std::vector<std::vector<glm::vec2>>& outPhase2D, std::vector<std::vector<float>>& outAmplitude, 
        std::vector<std::vector<float>>& outFrequency);

    std::vector<glm::quat> ConvertRotationsToLocalSpace(const std::vector<glm::quat>& relativeJointRots);

    void DebugWriteOutputToFile(const std::vector<float> data, bool out);

    std::vector<glm::vec3> GetRelativeJointVel(const std::vector<glm::vec3>& globalJointVel, const glm::mat4& root);

    std::vector<glm::vec3>  GetRelativeJointPos(const std::vector<glm::vec3>& globalJointPos, const glm::mat4& root);

    std::vector<glm::quat> GetRelativeJointRot(const std::vector<glm::quat>& globalJointRot, const glm::mat4& root);

    void GetGlobalJointPosition(std::vector<glm::vec3>& globalJointPos, const glm::mat4& root);
    void GetGlobalJointRotation(std::vector<glm::quat>& globalJointRot, const glm::mat4& root);
    void GetGlobalJointVelocity(std::vector<glm::vec3>& globalJointVel, const glm::mat4& root);

    int sampleCount() { return pastKeys + futureKeys + 1; }

    glm::vec2 Calc2DPhase(float phaseValue, float amplitude);

    glm::vec2 Update2DPhase(float amplitude, float frequency, glm::vec2 current, glm::vec2 next, float minAmplitude);

    float CalcPhaseValue(glm::vec2 phase);

    glm::mat4 BuildRootTransform(const glm::vec3& pos, const glm::quat& rot);

  

};