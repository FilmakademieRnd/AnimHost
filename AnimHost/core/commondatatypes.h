#ifndef COMMONDATATYPES_H
#define COMMONDATATYPES_H

#include <QObject>
#include <QString>

#define NODE_EDITOR_SHARED 1

#include "QtNodes/NodeData"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class HumanoidBones
{
    // Spine bones
    QVariantList spine;
    QVariantList chest;
    QVariantList upperChest;

    // Head bone
    QVariantList head;

    // Arm bones
    QVariantList leftShoulder;
    QVariantList leftUpperArm;
    QVariantList leftLowerArm;
    QVariantList leftHand;

    QVariantList rightShoulder;
    QVariantList rightUpperArm;
    QVariantList rightLowerArm;
    QVariantList rightHand;

    // Leg bones
    QVariantList leftUpperLeg;
    QVariantList leftLowerLeg;
    QVariantList leftFoot;
    QVariantList leftToes;

    QVariantList rightUpperLeg;
    QVariantList rightLowerLeg;
    QVariantList rightFoot;
    QVariantList rightToes;
};
Q_DECLARE_METATYPE(HumanoidBones)

//
// Qt Node Editor Data
//

//Float
class FloatData : public NodeData
{
public:
    FloatData()
        : _number(0.0)
    {}

    FloatData(float const number)
        : _number(number)
    {}

    NodeDataType type() const override { return NodeDataType{"float", "Float"}; }

    float number() const { return _number; }

    QString numberAsText() const { return QString::number(_number, 'f'); }

private:
    float _number;
};

//Humanoid Bones
class HumanoidBonesData : public NodeData
{
public:
    HumanoidBonesData()
        : _humanoidBones()
    {}

    HumanoidBonesData(HumanoidBones const humanoidBones)
        : _humanoidBones(humanoidBones)
    {}

    NodeDataType type() const override { return NodeDataType{"humanoidBones", "HumanoidBones"}; }

    HumanoidBones humanoidBones() const { return _humanoidBones; }

private:
    HumanoidBones _humanoidBones;
};

#endif // COMMONDATATYPES_H
