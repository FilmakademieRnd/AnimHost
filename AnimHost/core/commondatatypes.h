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



#define COMMONDATA(id, name)  static QString getId() { return #id ; };\
    static QString getName() { return #name ; };\

//! 3D position keyframe of an animation with timestamp
struct ANIMHOSTCORESHARED_EXPORT KeyPosition
{
    float timeStamp;
    glm::vec3 position;
};

//! 3D rotation keyframe of an animation with timestamp
struct ANIMHOSTCORESHARED_EXPORT KeyRotation
{
    float timeStamp;
    glm::quat orientation;
};

//! 3D scale keyframe of an animation with timestamp
struct ANIMHOSTCORESHARED_EXPORT KeyScale
{
    float timeStamp;
    glm::vec3 scale;
};

//! Bone data container
//! Every bone has a string name, and automatically generated ID and a series of positions, rotations and scales representing an animation.
//! Additionally, every node has a set resting transform
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
    
    // copy bone with information from single bone
    Bone(const Bone& o, int frame);
    
    Bone();

    glm::mat4 GetRestingTransform() { return mRestingTransform; };

    glm::quat GetOrientation(int frame) const;

    glm::vec3 GetPosition(int frame) const;

    glm::vec3 GetScale(int frame) const;

    COMMONDATA(bone, Bone)

};
Q_DECLARE_METATYPE(Bone)

//! Skeleton holds maps to retreive bone names given an ID and vice versa
//! It also stores the bone hierarchy (every bone ID is associated with an array of the IDs of its own children)
//! Additional info (don't know if still relevant)
class ANIMHOSTCORESHARED_EXPORT Skeleton
{
public:
    std::map<std::string, int> bone_names;
    std::map<int, std::string> bone_names_reverse;
    std::map<int, std::vector<int>> bone_hierarchy;

    int mAnimationDataSize = 0;
    int mNumBones = 0;
    int mRotationSize = 4;
    int mNumKeyFrames = 0;
    int mFrameOffset = 0;

private:


public:
    Skeleton() {};
    ~Skeleton() {};

    COMMONDATA(skeleton, Skeleton)
};
Q_DECLARE_METATYPE(Skeleton)

//! Animation data structure:
//! Duration in seconds and in frames
//! Array of Bone instances (sorted by ID) -> Bone instances contain sequences of pos/rot/sca of a specific bone (i.e. bone motions - see line 54 to 90)
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

        qDebug() << "Animation()";
    };

    Animation(const Animation& o) : mBones(o.mBones) { qDebug() << "Animation Copy"; };

    Animation(Animation&& o) noexcept : mBones(std::move(o.mBones)) { qDebug() << "Animation Move"; };

    ~Animation() {};

    COMMONDATA(animation, Animation)

};
Q_DECLARE_METATYPE(Animation)
Q_DECLARE_METATYPE(std::shared_ptr<Animation>)


//! Positions of every character joint relative to Character Space with relative timestamp
class ANIMHOSTCORESHARED_EXPORT Pose
{
    float timeStamp;

public:

    std::vector<glm::vec3> mPositionData;

public:
    Pose() {
        qDebug() << "Pose ()"; 
        mPositionData = std::vector<glm::vec3>();
    };
    ~Pose() {};
    Pose(const Pose& o) : mPositionData(o.mPositionData) { qDebug() << "Pose Copy"; };


    COMMONDATA(pose, Pose)

    
};

Q_DECLARE_METATYPE(Pose)
Q_DECLARE_METATYPE(std::shared_ptr<Pose>)

//! Sequence of poses
class ANIMHOSTCORESHARED_EXPORT PoseSequence
{
public:

    std::vector<Pose> mPoseSequence;
public:

    PoseSequence() { qDebug() << "PoseSequence()"; };

    COMMONDATA(poseSequence, PoseSequence)


};

Q_DECLARE_METATYPE(PoseSequence)
Q_DECLARE_METATYPE(std::shared_ptr<PoseSequence>)



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

    COMMONDATA(humanoidbones, HumanoidBones)


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
