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

 

#include "JointVelocityPlugin.h"
#include "../../core/commondatatypes.h"


#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>

JointVelocityPlugin::JointVelocityPlugin()
{
    qDebug() << "JointVelocityPlugin created";

    //Data
    inputs.append(QMetaType::fromName("PoseSequence"));
    inputs.append(QMetaType::fromName("Animation"));

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
    auto animationIn = in[1].value<std::shared_ptr<Animation>>();

    auto jointVelocitySequence = std::make_shared<JointVelocitySequence>();

    jointVelocitySequence->dataSetID = poseSequenceIn->dataSetID;
    jointVelocitySequence->sourceName = poseSequenceIn->sourceName;
    jointVelocitySequence->sequenceID = poseSequenceIn->sequenceID;

    //std::vector<glm::vec3> temp(poseSequenceIn->mPoseSequence[0].mPositionData.size(), glm::vec3(0.0f, 0.0f, 0.0f));
    //qDebug() << "Sequence Size: " << poseSequenceIn->mPoseSequence.size();

    //for (int frame = 0; frame < poseSequenceIn->mPoseSequence.size(); frame++) {
    //    
    //    jointVelocitySequence->mJointVelocitySequence.push_back(JointVelocity());
    //    // Special Case First Frame
    //    for (int bone = 0; bone < poseSequenceIn->mPoseSequence[frame].mPositionData.size(); bone++) {

    //        glm::vec3 currentPos = poseSequenceIn->mPoseSequence[frame].mPositionData[bone] / 100.f;
    //        
    //        glm::vec3 vel =  currentPos - temp[bone];

    //        vel = vel / (1.f / 60);

    //        jointVelocitySequence->mJointVelocitySequence[frame].mJointVelocity.push_back(vel);

    //        temp[bone] = currentPos;
    //    }
    //}

 



    // Do new Stuff

    for (int frame = 0; frame < poseSequenceIn->mPoseSequence.size(); frame++) {
        jointVelocitySequence->mJointVelocitySequence.push_back(JointVelocity());

     
        glm::mat4x4 prevTRS = animationIn->mBones[0].GetTransform(glm::max(0, frame - 1));
        glm::mat4x4 currTRS = animationIn->mBones[0].GetTransform(frame);

        glm::mat4x4 inv_prevTRS = glm::inverse(prevTRS);
        glm::mat4x4 inv_currTRS = glm::inverse(currTRS);
           
        for (int bone = 0; bone < poseSequenceIn->mPoseSequence[frame].mPositionData.size(); bone++) {

            glm::vec3 prevPos = inv_prevTRS * glm::vec4(poseSequenceIn->mPoseSequence[glm::max(0, frame - 1)].mPositionData[bone],1);
            glm::vec3 currPos = inv_currTRS * glm::vec4(poseSequenceIn->mPoseSequence[frame].mPositionData[bone],1);
            
            glm::vec3 v  =  (currPos - prevPos) / (1.f / 60.f); // Velociy Unit: m/s
            jointVelocitySequence->mJointVelocitySequence[frame].mJointVelocity.push_back(v/ 100.f);
           
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
