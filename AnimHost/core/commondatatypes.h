#ifndef COMMONDATATYPES_H
#define COMMONDATATYPES_H

#include <QtCore/qglobal.h>

#if defined(ANIMHOSTCORE_LIBRARY)
#define ANIMHOSTCORESHARED_EXPORT Q_DECL_EXPORT
#else
#define ANIMHOSTCORESHARED_EXPORT Q_DECL_IMPORT
#endif

#include <QObject>
#include <QString>
#include <QQuaternion>
#include <QMetaType>
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

#define NODE_EDITOR_SHARED 1

#include "QtNodes/NodeData"

using QtNodes::NodeData;
using QtNodes::NodeDataType;


struct ANIMHOSTCORESHARED_EXPORT KeyPosition
{
    float timeStamp;
    glm::vec3 position;
};

struct ANIMHOSTCORESHARED_EXPORT KeyRotation
{
    float timeStamp;
    glm::quat orientation;
};

struct ANIMHOSTCORESHARED_EXPORT KeyScale
{
    float timeStamp;
    glm::vec3 scale;
};

class ANIMHOSTCORESHARED_EXPORT Bone
{
public:
    int mID;
    int mNumKeysPosition;
    int mNumKeysRotation;
    int mNumKeysScale;

    std::string mName;
    std::vector<KeyPosition> mPositonKeys;
    std::vector<KeyRotation> mRotationKeys;
    std::vector<KeyScale> mScaleKeys;
    glm::mat4 mRestingTransform;

public:
    Bone(std::string name, int id, int numPos, int numRot, int numScl, glm::mat4 rest);
    Bone();

    glm::mat4 GetRestingTransform() { return mRestingTransform; };

    glm::quat GetOrientation(int frame);

    glm::vec3 GetPosition(int frame);

    glm::vec3 GetScale(int frame);

};

class ANIMHOSTCORESHARED_EXPORT Skeleton
{
public:
    std::map<std::string, int> bone_names;
    std::map<int, std::string> bone_names_reverse;
    std::map<int, std::vector<int>> bone_hierarchy;

    int mAnimationDataSize;
    int mNumBones;
    int mRotationSize = 4;
    int mNumKeyFrames;
    int mFrameOffset;

private:
    //void IndexSkelettonHirarchy(aiNode* pNode, int* currentBoneCount);


public:

    //Skeleton(aiNode* pNodes);

    //static void WriteToCSV(std::string filename, const Skeleton& pSkeleton, const AnimatedPoseData& mPoseData);
    //static void LoadAnimationData(aiAnimation* pASSIMPAnimation, Skeleton* pSkeleton, Animation* pAnimation, aiNode*);
    //static void BuildPose(Skeleton* pSkeleton, int frame, Animation* pAnimation, AnimatedPoseData& pAnimatedPoseData);

};

class ANIMHOSTCORESHARED_EXPORT Animation
{
public:
    float mDuration = 0.0;

    int mDurationFrames = 0;
    std::vector<Bone> mBones;


    //void SetRestingPosition(const aiNode& pNode, const Skeleton& pSkeleton);

    Animation()
    {
        mBones = std::vector<Bone>();
    };

};




class ANIMHOSTCORESHARED_EXPORT Pose
{
    float timeStamp;
    std::vector<glm::vec3> mPositionData;
public:
    Pose() {};
    ~Pose() {};
    Pose(const Pose&) {};
};

Q_DECLARE_METATYPE(Pose)



//!
//! \brief The prototype of a HumanoidBones class
//!
class ANIMHOSTCORESHARED_EXPORT HumanoidBones
{
public:
    HumanoidBones() { qDebug() << "HumanBones Hi!"; };
    HumanoidBones(const HumanoidBones& t);
    ~HumanoidBones() { qDebug() << "HumanBones Bye!!"; };

    void SetSpine(QQuaternion in) { spine = in; };
private:
    // Spine bones
    QQuaternion spine;
    QQuaternion chest;
    QQuaternion upperChest;

    // Head bone
    QQuaternion head;

    // Arm bones
    QQuaternion leftShoulder;
    QQuaternion leftUpperArm;
    QQuaternion leftLowerArm;
    QQuaternion leftHand;

    QQuaternion rightShoulder;
    QQuaternion rightUpperArm;
    QQuaternion rightLowerArm;
    QQuaternion rightHand;

    // Leg bones
    QQuaternion leftUpperLeg;
    QQuaternion leftLowerLeg;
    QQuaternion leftFoot;
    QQuaternion leftToes;

    QQuaternion rightUpperLeg;
    QQuaternion rightLowerLeg;
    QQuaternion rightFoot;
    QQuaternion rightToes;
};
//add HumanoidBones class as type to QVariants
Q_DECLARE_METATYPE(HumanoidBones)

//
// Qt Node Editor Data
//





#endif // COMMONDATATYPES_H
