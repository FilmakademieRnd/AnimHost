#ifndef NODEDATATYPES_H
#define NODEDATATYPES_H


#define NODE_EDITOR_SHARED 1
#include "QtNodes/NodeData"
#include "commondatatypes.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;


class ANIMHOSTCORESHARED_EXPORT AnimNodeData : public NodeData
{
public:
  
    virtual QVariant getVariant() const = 0;
    static NodeDataType staticType() { return NodeDataType{ "animNodeData", "AnimNodeData" }; };

};

//Float
class ANIMHOSTCORESHARED_EXPORT FloatData : public AnimNodeData
{
public:
    FloatData()
        : _number(0.0)
    {}

    FloatData(float const number)
        : _number(number)
    {}

    NodeDataType type() const override { return staticType(); }
    static NodeDataType staticType() { return NodeDataType{ "float", "Float" }; }

    float number() const { return _number; }

    QString numberAsText() const { return QString::number(_number, 'f'); }

    QVariant getVariant() const override { return QVariant(_number); }


private:
    float _number;
};

//Float
class ANIMHOSTCORESHARED_EXPORT IntData : public AnimNodeData
{
public:
    IntData()
        : _number(0)
    {}

    IntData(int const number)
        : _number(number)
    {}

    NodeDataType type() const override { return staticType(); }
    static NodeDataType staticType() { return NodeDataType{ "int", "Int" }; }

    float number() const { return _number; }

    QString numberAsText() const { return QString::number(_number); }

    QVariant getVariant() const override { return QVariant(_number); }

private:
    int _number;
};

//Humanoid Bones
class ANIMHOSTCORESHARED_EXPORT HumanoidBonesData : public AnimNodeData
{
public:
    HumanoidBonesData()
    {}

    HumanoidBonesData(HumanoidBones const humanoidBones)
        : _humanoidBones(humanoidBones)
    {}

    NodeDataType type() const override { return staticType(); }

    static NodeDataType staticType() { return NodeDataType{ "humanoidBones", "HumanoidBones" }; }

    //static NodeDataType staticType() { return NodeDataType{ "humanoidBones", "HumanoidBones" }; }

    HumanoidBones humanoidBones() const { return _humanoidBones; }

    QVariant getVariant() const override { return QVariant::fromValue(_humanoidBones); }
    
    HumanoidBones _humanoidBones;

private:
    
};

//Pose
class ANIMHOSTCORESHARED_EXPORT PoseNodeData : public AnimNodeData
{
public:
    PoseNodeData()
        : _pose()
    {}


    PoseNodeData(Pose const pose)
        : _pose(pose)
    {}

    NodeDataType type() const override { return staticType(); }

    static NodeDataType staticType() { return NodeDataType{ "pose", "Pose" }; }

    Pose pose() const { return _pose; }

    QVariant getVariant() const override { return QVariant::fromValue(_pose); }

    //QString poseAsText() const { return QString::number(_number, 'f'); }

private:
    Pose _pose;
};



class ANIMHOSTCORESHARED_EXPORT SkeletonNodeData : public AnimNodeData
{
public:
    SkeletonNodeData()
        : _skeleton()
    {}


    SkeletonNodeData(Skeleton const skeleton)
        : _skeleton(skeleton)
    {}

    NodeDataType type() const override { return staticType(); }

    static NodeDataType staticType() { return NodeDataType{ "skeleton", "Skeleton" }; }

    Skeleton* skeleton() { return &_skeleton; }

    QVariant getVariant() const override { return QVariant::fromValue(_skeleton); }

    //QString poseAsText() const { return QString::number(_number, 'f'); }

private:
    Skeleton _skeleton;
};

class ANIMHOSTCORESHARED_EXPORT AnimationNodeData : public AnimNodeData
{
public:
    AnimationNodeData()
        : _animation()
    {}


    AnimationNodeData(Animation const animation)
        : _animation(animation)
    {}

    NodeDataType type() const override { return staticType(); }

    static NodeDataType staticType() { return NodeDataType{ "animation", "Animation" }; }

    Animation* animation() { return &_animation; }

    QVariant getVariant() const override { return QVariant::fromValue(_animation); }

    //QString poseAsText() const { return QString::number(_number, 'f'); }

private:
    Animation _animation;
};
#endif // NODEDATATYPES_H
