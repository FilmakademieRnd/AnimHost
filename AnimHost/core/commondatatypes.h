#ifndef COMMONDATATYPES_H
#define COMMONDATATYPES_H

#include <QObject>
#include <QString>
#include <QQuaternion>
#include <vector>
#include <glm/glm.hpp>

#define NODE_EDITOR_SHARED 1

#include "QtNodes/NodeData"

using QtNodes::NodeData;
using QtNodes::NodeDataType;




class Pose
{
    float timeStamp;
    std::vector<glm::vec3> mPositionData;
};

Q_DECLARE_METATYPE(Pose)











//!
//! \brief The prototype of a HumanoidBones class
//!
class HumanoidBones
{
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
