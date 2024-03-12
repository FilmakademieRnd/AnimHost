#include "GNNPlugin_global.h"

#include "HistoryBuffer.h"
#include "OnnxModel.h"

class GNNPLUGINSHARED_EXPORT GNNController
{
private:


    int pastKeys = 6;
    int futureKeys = 5;

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
    std::vector<float> Phase;
    std::vector<float> Amplitude;

    std::vector<glm::vec2> phase2D;

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


    //normalisation

    std::vector<float> stdIn;
    std::vector<float> meanIn;

    std::vector<float> stdOut;
    std::vector<float> meanOut;

    //in & output tensors
    std::vector<float> input_values;
    std::vector<float> output_values;

    int currentPivot = 0;


    /* Neural Network */
    std::unique_ptr<OnnxModel> network;
    QString NetworkModelPath = "C:\\DEV\\AI4Animation\\AI4Animation\\SIGGRAPH_2022\\PyTorch\\GNN\\Training\\147.onnx";

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

private:

    

    void BuildInputTensor(const std::vector<glm::vec2>& pos, const std::vector<glm::vec2>& dir, const std::vector<glm::vec2>& vel, const std::vector<float>& speed,
        const std::vector<glm::vec3>& jointPos, const std::vector<glm::quat>& jointRot, const std::vector<glm::vec3>& jointVel);
    
    void readOutput();

    void DebugWriteOutputToFile(const std::vector<float> data, bool out);

    std::vector<glm::vec3> GetRelativeJointVel(const std::vector<glm::vec3>& globalJointVel, const glm::mat4& root);

    std::vector<glm::vec3>  GetRelativeJointPos(const std::vector<glm::vec3>& globalJointPos, const glm::mat4& root);

    std::vector<glm::quat> GetRelativeJointRot(const std::vector<glm::quat>& globalJointRot, const glm::mat4& root);

    int sampleCount() { return pastKeys + futureKeys + 1; }

    glm::vec3 PositionTo(const glm::vec3& from, const glm::mat4& to);
    glm::vec2 PositionTo(const glm::vec2& from, const glm::mat4& to);
    glm::vec3 DirectionTo(const glm::vec3& from, const glm::mat4& to);
    glm::vec2 Direction2DTo(const glm::vec3& from, const glm::mat4& to);

    glm::vec2 Calc2DPhase(float phaseValue, float amplitude);

    glm::vec2 Update2DPhase(float amplitude, float frequency, glm::vec2 current, glm::vec2 next, float minAmplitude);

    float CalcPhaseValue(glm::vec2 phase);

    glm::mat4 BuildRootTransform(const glm::vec3& pos, const glm::quat& rot);

  

};