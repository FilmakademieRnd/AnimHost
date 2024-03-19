#ifndef COMMONDATATYPES_H
#define COMMONDATATYPES_H

#include "animhostcore_global.h"

#include <QObject>
#include <QString>
#include <QQuaternion>
#include <QMetaType>
#include <QUuid>
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
    QString dataSetID = "";

    int sequenceID = -1;

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

    glm::mat4 GetTransform(int frame) const;

    COMMONDATA(bone, Bone)

};
Q_DECLARE_METATYPE(std::shared_ptr<Bone>)

/**
 * @class Skeleton
 * @brief A class that represents a skeleton in an animation.
 *
 * This class holds maps to retrieve bone names given an ID and vice versa.
 * It also stores the bone hierarchy (every bone ID is associated with an array of the IDs of its own children).
 */
class ANIMHOSTCORESHARED_EXPORT Skeleton
{
public:
    std::map<std::string, int> bone_names; ///< Map from bone names to IDs.
    std::map<int, std::string> bone_names_reverse; ///< Map from bone IDs to names.
    std::map<int, std::vector<int>> bone_hierarchy; ///< Map from bone IDs to a vector of its children's IDs.

    int mAnimationDataSize = 0; ///< Size of the animation data.
    int mNumBones = 0; ///< Number of bones in the skeleton.
    int mRotationSize = 4; ///< Size of the rotation data.
    int mNumKeyFrames = 0; ///< Number of keyframes in the animation.
    int mFrameOffset = 0; ///< Offset for the frames in the animation.

public:
    /**
     * @brief Default constructor for the Skeleton class.
     */
    Skeleton() {};

    /**
     * @brief Destructor for the Skeleton class.
     */
    ~Skeleton() {};

    COMMONDATA(skeleton, Skeleton)
};
Q_DECLARE_METATYPE(std::shared_ptr<Skeleton>)

/**
 * @class Animation
 * @brief A class that represents an animation sequence.
 *
 * This class inherits from the Sequence class and contains a vector of Bone objects.
 * Each Bone object represents a bone in the animation and contains sequences of positions, rotations, and scales.
 * The Animation class also stores the duration of the animation in seconds and frames.
 */
class ANIMHOSTCORESHARED_EXPORT Animation : public Sequence
{
public:
    float mDuration = 0.0;

    int mDurationFrames = 0;
    std::vector<Bone> mBones;


    /**
     * @brief Default constructor for the Animation class.
     *
     * This constructor initializes the bones vector as an empty vector.
     */
    Animation()
    {
        mBones = std::vector<Bone>();

        qDebug() << "Animation()";
    };

    /**
     * @brief Copy constructor for the Animation class.
     *
     * This constructor creates a new Animation object as a copy of an existing one.
     *
     * @param o The Animation object to copy.
     */
    Animation(const Animation& o) : mBones(o.mBones), mDurationFrames(o.mDurationFrames), mDuration(o.mDuration) { qDebug() << "Animation Copy"; };

    /**
     * @brief Move constructor for the Animation class.
     *
     * This constructor creates a new Animation object by moving the data from an existing one.
     *
     * @param o The Animation object to move.
     */
    Animation(Animation&& o) noexcept : mBones(std::move(o.mBones)) { qDebug() << "Animation Move"; };

    /**
     * @brief Destructor for the Animation class.
     */
    ~Animation() {};

    COMMONDATA(animation, Animation)

};
Q_DECLARE_METATYPE(std::shared_ptr<Animation>)


/**
 * @class JointVelocity
 * @brief A class that represents the velocity of every character joint during a specific timestamp.
 *
 * This class calculates the velocity based on global position V = Xt - X(t-1).
 */
class ANIMHOSTCORESHARED_EXPORT JointVelocity
{
    float timeStamp;
        
public:

    std::vector<glm::vec3> mJointVelocity;

public: 
    /**
     * @brief Default constructor for the JointVelocity class.
     *
     * This constructor initializes the joint velocity vector as an empty vector.
     */
    JointVelocity() : timeStamp(0.0f), mJointVelocity(std::vector<glm::vec3>()) {};
    
    COMMONDATA(jointVelocity, JointVelocity)

};
Q_DECLARE_METATYPE(std::shared_ptr<JointVelocity>)

/**
 * @class JointVelocitySequence
 * @brief A class that represents a sequence of joint velocities in an animation.
 *
 * This class inherits from the Sequence class and contains a vector of JointVelocity objects.
 * It provides methods to get the velocity of a bone at a specific frame.
 */
class ANIMHOSTCORESHARED_EXPORT JointVelocitySequence : public Sequence
{
public:
    std::vector<JointVelocity> mJointVelocitySequence; ///< The sequence of joint velocities.


public:
    JointVelocitySequence() {};

    /**
     * @brief Get the 2D velocity of the root bone at a specific frame.
     *
     * This function returns the x and z components of the root bone's velocity,
     * effectively projecting the 3D velocity onto the xz-plane.
     *
     * @param FrameIndex The index of the frame.
     * @return The x and z components of the root bone's velocity at the specified frame.
     */
    glm::vec2 GetRootVelocityAtFrame2D(int FrameIndex) {

        return glm::vec2(mJointVelocitySequence[FrameIndex].mJointVelocity[0].x, 
            mJointVelocitySequence[FrameIndex].mJointVelocity[0].z);
    }

    /**
     * @brief Get the 3D velocity of the root bone at a specific frame.
     *
     * This function returns the 3D velocity of the root bone at the specified frame.
     *
     * @param FrameIndex The index of the frame.
     * @return The 3D velocity of the root bone at the specified frame.
     */
    glm::vec3 GetRootVelocityAtFrame(int FrameIndex) {
        return GetVelocityAtFrame(FrameIndex,0);
    }


    /**
     * @brief Get the 3D velocity of a bone at a specific frame.
     *
     * This function returns the 3D velocity of the bone at the specified frame.
     *
     * @param FrameIndex The index of the frame.
     * @param BoneIndx The index of the bone.
     * @return The 3D velocity of the bone at the specified frame.
     */
    glm::vec3 GetVelocityAtFrame(int FrameIndex, int BoneIndx) {
        return glm::vec3(mJointVelocitySequence[FrameIndex].mJointVelocity[BoneIndx]);
    }


    COMMONDATA(jointVelocitySequence, JointVelocitySequence)


};
Q_DECLARE_METATYPE(std::shared_ptr<JointVelocitySequence>)




/**
 * @class Pose
 * @brief A class that represents a pose in an animation.
 *
 * This class holds the positions of every character in world space during a specific timestamp.
 */
    class ANIMHOSTCORESHARED_EXPORT Pose
{
    float timeStamp = -1; ///< The timestamp of the pose.

public:

    std::vector<glm::vec3> mPositionData; ///< The position data of the pose.

public:
    /**
     * @brief Default constructor for the Pose class.
     *
     * This constructor initializes the position data as an empty vector.
     */
    Pose() {
        mPositionData = std::vector<glm::vec3>();
    };

    COMMONDATA(pose, Pose)
};
Q_DECLARE_METATYPE(std::shared_ptr<Pose>)

/**
 * @class PoseSequence
 * @brief A class that represents a sequence of poses in an animation.
 *
 * This class inherits from the Sequence class and contains a vector of Pose objects.
 * It provides methods to get the position of a bone at a specific frame.
 */
class ANIMHOSTCORESHARED_EXPORT PoseSequence : public Sequence
{
public:

    std::vector<Pose> mPoseSequence; ///< The sequence of poses.

public:

    PoseSequence() { qDebug() << "PoseSequence()"; };


    /**
     * @brief Get the position of the root bone at a specific frame.
     *
     * This function returns the x and z coordinates of the root bone's position,
     * effectively projecting the 3D position onto the xz-plane.
     *
     * @param FrameIndex The index of the frame.
     * @return The x and z coordinates of the root bone's position at the specified frame.
     */
    glm::vec2 GetRootPositionAtFrame(int FrameIndex) {

        // We assume the root bone is always at index 0 in the positional data.
        //Support different coordinate Systems
        return GetPositionAtFrame(FrameIndex, 0);

    }

    /**
     * @brief Get the position of a bone at a specific frame.
     *
     * This function returns the x and z coordinates of the bone's position,
     * effectively projecting the 3D position onto the xz-plane. 
     *
     * @param FrameIndex The index of the frame.
     * @param boneIndex The index of the bone.
     * @return The x and z coordinates of the bone's position at the specified frame.
     */
    glm::vec2 GetPositionAtFrame(int FrameIndex, int boneIndex) {

        if (FrameIndex < mPoseSequence.size() && boneIndex < mPoseSequence[FrameIndex].mPositionData.size()) {
			return glm::vec2(mPoseSequence[FrameIndex].mPositionData[boneIndex].x,
                				mPoseSequence[FrameIndex].mPositionData[boneIndex].z);
        }
        else {
            qDebug() << "Error::GetPositionAtFrame: FrameIndex or boneIndex out of range";
			return glm::vec2(0, 0);
		}   
    }

    /**
     * @brief Get the 3D position of a bone at a specific frame.
     *
     * This function returns the 3D position of the bone at the specified frame.
     * It checks if the FrameIndex and boneIndex are within the bounds of the data structure.
     * If either index is out of bounds, it logs an error and returns a zero vector.
     *
     * @param FrameIndex The index of the frame.
     * @param boneIndex The index of the bone.
     * @return The 3D position of the bone at the specified frame.
     */
    glm::vec3 GetPositionAtFrame3D(int FrameIndex, int boneIndex) {

        if (FrameIndex < mPoseSequence.size() && boneIndex < mPoseSequence[FrameIndex].mPositionData.size()) {
			return mPoseSequence[FrameIndex].mPositionData[boneIndex];
		}
        else {
			qDebug() << "Error::GetPositionAtFrame3D: FrameIndex or boneIndex out of range";
			return glm::vec3(0, 0, 0);
		}
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

class ANIMHOSTCORESHARED_EXPORT ControlPoint {
    public:
    int frameTimestamp;

    glm::vec3 position;
    glm::quat lookAt;
    float velocity;

    //int styleLabel;

    public:
    ControlPoint() { qDebug() << "ControlPoint()"; };

    COMMONDATA(controlPoint, ControlPoint)

};
Q_DECLARE_METATYPE(ControlPoint)
Q_DECLARE_METATYPE(std::shared_ptr<ControlPoint>)

//! Data structure describing the path that the character has to follow
/*!
* It contains a sequence of waypoints and the total amount of frames that are going to be generated by
* the In-betweening Neural Network
*/
class ANIMHOSTCORESHARED_EXPORT ControlPath : public Sequence {
    public:
    int frameCount;
    std::vector<ControlPoint> mControlPath;

    public:
    ControlPath() : mControlPath {} { qDebug() << "ControlPath()"; };

    void CreateSpline();

    COMMONDATA(controlPath, ControlPath)

};
Q_DECLARE_METATYPE(ControlPath)
Q_DECLARE_METATYPE(std::shared_ptr<ControlPath>)

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

    ControlPath controlPath;

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
