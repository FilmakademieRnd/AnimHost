#include "GNNPlugin_global.h"

#include "HistoryBuffer.h"
#include "OnnxModel.h"
#include "PhaseSequence.h"

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

    GNNController();
    void prepareInput();

    void InitDummyData();

    void SetSkeleton(std::shared_ptr<Skeleton> skel);

    void SetAnimationIn(std::shared_ptr<Animation> anim);

    void SetControlPath(std::shared_ptr<ControlPath> path);

    std::shared_ptr<Animation> GetAnimationOut();

    void BuildAnimationSequence(const std::vector<std::vector<glm::quat>>& jointRotSequence);
    
 

private:

    

    void BuildInputTensor(const std::vector<glm::vec2>& pos, const std::vector<glm::vec2>& dir, const std::vector<glm::vec2>& vel, const std::vector<float>& speed,
        const std::vector<glm::vec3>& jointPos, const std::vector<glm::quat>& jointRot, const std::vector<glm::vec3>& jointVel);
    
    void  readOutput(const std::vector<float>& output_values, std::vector<glm::vec3>& outJointPosition,
        std::vector<glm::quat>& outJointRotation, std::vector<glm::vec3>& outJointVelocity,
        std::vector<std::vector<glm::vec2>>& outPhase2D, std::vector<std::vector<float>>& outAmplitude, std::vector<std::vector<float>>& outFrequency);

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