#ifndef COMMONDATATYPES_H
#define COMMONDATATYPES_H

#include "animhostcore_global.h"

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


//!Datacontainer for identifing proccessed and saved sequences 
class Sequence {

public:

    // Name of the motion source. may be populated by original filename of sequence or freely chosen "Take" name.
    QString sourceName = "";

    // Used as identifier to map data to Source Name
    int dataSetID = -1;
};


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
    glm::quat restingRotation;

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
Q_DECLARE_METATYPE(std::shared_ptr<Bone>)

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
Q_DECLARE_METATYPE(std::shared_ptr<Skeleton>)

//! Animation data structure:
//! Duration in seconds and in frames
//! Array of Bone instances (sorted by ID) -> Bone instances contain sequences of pos/rot/sca of a specific bone (i.e. bone motions - see line 54 to 90)
class ANIMHOSTCORESHARED_EXPORT Animation : public Sequence
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

    Animation(const Animation& o) : mBones(o.mBones), mDurationFrames(o.mDurationFrames), mDuration(o.mDuration) { qDebug() << "Animation Copy"; };

    Animation(Animation&& o) noexcept : mBones(std::move(o.mBones)) { qDebug() << "Animation Move"; };

    ~Animation() {};

    COMMONDATA(animation, Animation)

};
Q_DECLARE_METATYPE(std::shared_ptr<Animation>)


//! Velocity of every character joint calculated based on global position V = Xt - X(t-1)
class ANIMHOSTCORESHARED_EXPORT JointVelocity
{
    float timeStamp;
        
public:

    std::vector<glm::vec3> mJointVelocity;

public: 
    JointVelocity() {
       mJointVelocity = std::vector<glm::vec3>();
    };
    
    COMMONDATA(jointVelocity, JointVelocity)

};
Q_DECLARE_METATYPE(std::shared_ptr<JointVelocity>)

//! Sequence of joint velocities
class ANIMHOSTCORESHARED_EXPORT JointVelocitySequence : public Sequence
{
public:
    std::vector<JointVelocity> mJointVelocitySequence;

public:
    JointVelocitySequence() {};

    glm::vec2 GetRootVelocityAtFrame(int FrameIndex) {

        // We assume the root bone is always at index 0 in the velocity data.
        //Support different coordinate Systems
        return glm::vec2(mJointVelocitySequence[FrameIndex].mJointVelocity[0].x, 
            mJointVelocitySequence[FrameIndex].mJointVelocity[0].z);
    }


    COMMONDATA(jointVelocitySequence, JointVelocitySequence)


};
Q_DECLARE_METATYPE(std::shared_ptr<JointVelocitySequence>)




//! Positions of every character joint relative to Character Space with relative timestamp
class ANIMHOSTCORESHARED_EXPORT Pose
{
    float timeStamp = -1;

public:

    std::vector<glm::vec3> mPositionData;

public:
    Pose() {
        mPositionData = std::vector<glm::vec3>();
    };

    //Pose(const Pose& o) : mPositionData(o.mPositionData) {};


    COMMONDATA(pose, Pose)

    
};
Q_DECLARE_METATYPE(std::shared_ptr<Pose>)

//! Sequence of poses
class ANIMHOSTCORESHARED_EXPORT PoseSequence : public Sequence
{
public:

    std::vector<Pose> mPoseSequence;
public:

    PoseSequence() { qDebug() << "PoseSequence()"; };


    glm::vec2 GetRootPositionAtFrame(int FrameIndex) {

        // We assume the root bone is always at index 0 in the positional data.
        //Support different coordinate Systems
        return glm::vec2(mPoseSequence[FrameIndex].mPositionData[0].x, 
            mPoseSequence[FrameIndex].mPositionData[0].z);
            
            
    }

    COMMONDATA(poseSequence, PoseSequence)
};
Q_DECLARE_METATYPE(std::shared_ptr<PoseSequence>)


class ANIMHOSTCORESHARED_EXPORT RunSignal
{

public:

    RunSignal() { qDebug() << "RunSignal()"; };

    COMMONDATA(runSignal, Run)


};
Q_DECLARE_METATYPE(std::shared_ptr<RunSignal>)

class ANIMHOSTCORESHARED_EXPORT SceneNodeObject {
    public:
    int sceneObjectID; //! ID of the object 
    int characterRootID;
    std::string objectName;

    glm::vec3 pos;
    glm::vec4 rot;
    glm::vec3 scl;

    SceneNodeObject() : sceneObjectID { 0 }, characterRootID { 0 }, objectName { "" }, pos {glm::vec3(0)}, rot { glm::vec4(0) }, scl { glm::vec3(0) } {};

    SceneNodeObject(int soID, int crID, std::string name) :
        sceneObjectID { soID }, characterRootID { crID }, objectName { name }, pos { glm::vec3(0) }, rot { glm::vec4(0) }, scl { glm::vec3(0) } {};

    SceneNodeObject(int soID, int crID, std::string name, glm::vec3 p, glm::vec4 r, glm::vec3 s) :
        sceneObjectID { soID }, characterRootID { crID }, objectName { name }, pos { p }, rot { r }, scl { s } {};

    COMMONDATA(sceneNodeObject, SceneNodeObject)
};
Q_DECLARE_METATYPE(SceneNodeObject)
Q_DECLARE_METATYPE(std::shared_ptr<SceneNodeObject>)

class ANIMHOSTCORESHARED_EXPORT SkinnedMeshComponent {
    public:
    int id;
    glm::vec3 boundExtents;
    glm::vec3 boundCenter;
    std::vector <glm::mat4> bindPoses;
    std::vector<int> boneMapIDs;

    public:
    SkinnedMeshComponent() :
        id {0}, boundExtents { glm::vec3(0) }, boundCenter { glm::vec3(0) }, bindPoses {}, boneMapIDs {} {};

    COMMONDATA(skinnedMeshComponent, SkinnedMeshComponent)
};
Q_DECLARE_METATYPE(SkinnedMeshComponent)
Q_DECLARE_METATYPE(std::shared_ptr<SkinnedMeshComponent>)

class ANIMHOSTCORESHARED_EXPORT CharacterObject :public SceneNodeObject {
    public:
    std::vector<int> boneMapping;
    std::vector<int> skeletonObjIDs;
    std::vector<glm::vec3> tposeBonePos;
    std::vector<glm::quat> tposeBoneRot;
    std::vector<glm::vec3> tposeBoneScale;
    
    std::vector<SkinnedMeshComponent> skinnedMeshList;

    public:
    CharacterObject(int soID, int oID, std::string name) :
        SceneNodeObject ( soID, oID, name ),
        boneMapping {}, skeletonObjIDs {},
        tposeBonePos {}, tposeBoneRot {}, tposeBoneScale {},
        skinnedMeshList {} {};
    
    CharacterObject() :
        SceneNodeObject(),
        boneMapping {}, skeletonObjIDs {},
        tposeBonePos {}, tposeBoneRot {}, tposeBoneScale {},
        skinnedMeshList {} {};

    void fill(const CharacterObject& _otherChObj) {
        sceneObjectID = _otherChObj.sceneObjectID;
        characterRootID = _otherChObj.characterRootID;
        objectName = _otherChObj.objectName;

        pos = _otherChObj.pos;
        rot = _otherChObj.rot;
        scl = _otherChObj.scl;
        
        boneMapping = _otherChObj.boneMapping;
        skeletonObjIDs = _otherChObj.skeletonObjIDs;
        tposeBonePos = _otherChObj.tposeBonePos;
        tposeBoneRot = _otherChObj.tposeBoneRot;
        tposeBoneScale = _otherChObj.tposeBoneScale;

        skinnedMeshList = _otherChObj.skinnedMeshList;
    }

    COMMONDATA(characterObject, CharacterObject)

};
Q_DECLARE_METATYPE(CharacterObject)
Q_DECLARE_METATYPE(std::shared_ptr<CharacterObject>)

class ANIMHOSTCORESHARED_EXPORT SceneNodeObjectSequence : public Sequence {
    public:

    std::vector<SceneNodeObject> mSceneNodeObjectSequence;

    public:
    SceneNodeObjectSequence() : mSceneNodeObjectSequence {} { qDebug() << "SceneNodeObjectSequence()"; };

    COMMONDATA(sceneNodeObjectSequence, SceneNodeObjectSequence)

};
Q_DECLARE_METATYPE(SceneNodeObjectSequence)
Q_DECLARE_METATYPE(std::shared_ptr<SceneNodeObjectSequence>)

class ANIMHOSTCORESHARED_EXPORT CharacterObjectSequence : public Sequence {
    public:

    std::vector<CharacterObject> mCharacterObjectSequence;

    public:
    CharacterObjectSequence() : mCharacterObjectSequence {} { qDebug() << "CharacterObjectSequence()"; };

    COMMONDATA(characterObjectSequence, CharacterObjectSequence)
      
};
Q_DECLARE_METATYPE(CharacterObjectSequence)
Q_DECLARE_METATYPE(std::shared_ptr<CharacterObjectSequence>)

#endif // COMMONDATATYPES_H
