#include "JointPositionPlugin.h"
#include <iostream>
#include <QFileInfo>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include "../../core/commondatatypes.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>

JointPositionPlugin::JointPositionPlugin()
{
    qDebug() << "JointPositionPlugin created";
 
    //Data
    inputs.append(QMetaType::fromName("Skeleton"));
    inputs.append(QMetaType::fromName("Animation"));

    outputs.append(QMetaType::fromName("PoseSequence"));
}

JointPositionPlugin::~JointPositionPlugin()
{
    qDebug() << "~JointPositionPlugin()";
}

// execute the main functionality of the plugin
void JointPositionPlugin::run(QVariantList in, QVariantList& out)
{
    qDebug() << "RUN Global Joint Position Calculation";
    //execute
    std::shared_ptr<Skeleton> skeleton = in[0].value<std::shared_ptr<Skeleton>>();
    std::shared_ptr<Animation> animation = in[1].value<std::shared_ptr<Animation>>();

    auto poseSequence = std::make_shared<PoseSequence>();
    int frame = 0;
  
    poseSequence->mPoseSequence = std::vector<Pose>(animation->mDurationFrames);


    std::function<void(glm::mat4, int)> lBuildPose;
    lBuildPose = [&](glm::mat4 currentT, int currentBone) {

        glm::mat4 transform = animation->mBones[currentBone].GetRestingTransform();

        glm::vec4 result = glm::vec4(0.0, 0.0, 0.0, 1.0);

        glm::quat orientation = animation->mBones[currentBone].GetOrientation(frame);
        glm::mat4 rotation = glm::toMat4(orientation);

        glm::vec3 scl = animation->mBones[currentBone].GetScale(frame);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), scl);

        glm::vec3 pos = animation->mBones[currentBone].GetPosition(frame);
        glm::mat4 translation (1.0f);
        if (currentBone == 0) {
            translation = glm::translate(translation, glm::vec3(0.0, pos.y, 0.0));
        }
        else {
            translation = glm::translate(translation, pos);
        }
        //glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);

        glm::mat4 local_transform = translation * rotation * scale;
  
        glm::mat4 globalT = currentT * local_transform;
        result = globalT * result;

        poseSequence->mPoseSequence[frame].mPositionData[currentBone] = result;
        if (currentBone == 0) {
            poseSequence->mPoseSequence[frame].mPositionData[currentBone] += glm::vec3(pos.x, 0, pos.z);
        }

        for (int i : skeleton->bone_hierarchy[currentBone]) {
            lBuildPose(globalT, i);
        }
    };


    for (int i = 0; i < animation->mDurationFrames; i++) {

        int initcurrentBone = 0;
        glm::mat4 initcurrentPos(1.0f);

        poseSequence->mPoseSequence[frame].mPositionData = std::vector<glm::vec3>(skeleton->mNumBones, glm::vec3(0.0));

        lBuildPose(initcurrentPos, initcurrentBone);

        frame++;
    }

    poseSequence->dataSetID = animation->dataSetID;
    poseSequence->sourceName = animation->sourceName;

    out.append(QVariant::fromValue(poseSequence));
}

QString JointPositionPlugin::category()
{
    return "Operator";
}

QList<QMetaType> JointPositionPlugin::inputTypes()
{
    return inputs;
}

QList<QMetaType> JointPositionPlugin::outputTypes()
{
    return outputs;
}
