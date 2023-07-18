#ifndef NODEDATATYPES_H
#define NODEDATATYPES_H


#define NODE_EDITOR_SHARED 1
#include "QtNodes/NodeData"
#include "commondatatypes.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;


class AnimNodeData : public NodeData
{
public:
  
    virtual QVariant getVariant() const = 0;

};

//Float
class FloatData : public AnimNodeData
{
public:
    FloatData()
        : _number(0.0)
    {}

    FloatData(float const number)
        : _number(number)
    {}

    NodeDataType type() const override { return NodeDataType{ "float", "Float" }; }

    float number() const { return _number; }

    QString numberAsText() const { return QString::number(_number, 'f'); }

    QVariant getVariant() const override { return QVariant(_number); }


private:
    float _number;
};

//Float
class IntData : public AnimNodeData
{
public:
    IntData()
        : _number(0)
    {}

    IntData(int const number)
        : _number(number)
    {}

    NodeDataType type() const override { return NodeDataType{ "int", "Int" }; }

    float number() const { return _number; }

    QString numberAsText() const { return QString::number(_number); }

    QVariant getVariant() const override { return QVariant(_number); }

private:
    int _number;
};

//Humanoid Bones
class HumanoidBonesData : public AnimNodeData
{
public:
    HumanoidBonesData()
        : _humanoidBones()
    {}

    HumanoidBonesData(HumanoidBones const humanoidBones)
        : _humanoidBones(humanoidBones)
    {}

    NodeDataType type() const override { return NodeDataType{ "humanoidBones", "HumanoidBones" }; }

    HumanoidBones humanoidBones() const { return _humanoidBones; }

    QVariant getVariant() const override { return QVariant::fromValue(_humanoidBones); }

private:
    HumanoidBones _humanoidBones;
};

//Pose
class PoseNodeData : public AnimNodeData
{
public:
    PoseNodeData()
        : _pose()
    {}

    PoseNodeData(Pose const pose)
        : _pose(pose)
    {}

    NodeDataType type() const override { return NodeDataType{"pose", "Pose"}; }

    Pose pose() const { return _pose; }

    QVariant getVariant() const override { return QVariant::fromValue(_pose); }

    //QString poseAsText() const { return QString::number(_number, 'f'); }

private:
    Pose _pose;
};

#endif // NODEDATATYPES_H
