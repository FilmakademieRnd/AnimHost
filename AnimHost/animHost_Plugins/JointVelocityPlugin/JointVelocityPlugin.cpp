
#include "JointVelocityPlugin.h"
#include "../../core/commondatatypes.h"

JointVelocityPlugin::JointVelocityPlugin()
{
    qDebug() << "JointVelocityPlugin created";

    //Data
    inputs.append(QMetaType::fromName("PoseSequence"));

    outputs.append(QMetaType::fromName("JointVelocitySequence"));
}

JointVelocityPlugin::~JointVelocityPlugin()
{
    qDebug() << "~JointVelocityPlugin()";
}

// execute the main functionality of the plugin
void JointVelocityPlugin::run(QVariantList in, QVariantList& out)
{
    qDebug() << "Running JointVelocityPlugin";

    auto poseSequenceIn = in[0].value<std::shared_ptr<PoseSequence>>();

    auto jointVelocitySequence = std::make_shared<JointVelocitySequence>();

    jointVelocitySequence->dataSetID = poseSequenceIn->dataSetID;
    jointVelocitySequence->sourceName = poseSequenceIn->sourceName;
    jointVelocitySequence->sequenceID = poseSequenceIn->sequenceID;

    std::vector<glm::vec3> temp(poseSequenceIn->mPoseSequence[0].mPositionData.size(), glm::vec3(0.0f, 0.0f, 0.0f));
    qDebug() << "Sequence Size: " << poseSequenceIn->mPoseSequence.size();

    for (int frame = 0; frame < poseSequenceIn->mPoseSequence.size(); frame++) {
        
        jointVelocitySequence->mJointVelocitySequence.push_back(JointVelocity());
        // Special Case First Frame
        for (int bone = 0; bone < poseSequenceIn->mPoseSequence[frame].mPositionData.size(); bone++) {

            glm::vec3 currentPos = poseSequenceIn->mPoseSequence[frame].mPositionData[bone] / 100.f;
            
            glm::vec3 vel =  currentPos - temp[bone];

            vel = vel / (1.f / 60);

            jointVelocitySequence->mJointVelocitySequence[frame].mJointVelocity.push_back(vel);

            temp[bone] = currentPos;
        }
    }

    out.append(QVariant::fromValue(jointVelocitySequence));
}

QString JointVelocityPlugin::category()
{
    return "Operator";
}

QList<QMetaType> JointVelocityPlugin::inputTypes()
{
    return inputs;
}

QList<QMetaType> JointVelocityPlugin::outputTypes()
{
    return outputs;
}
