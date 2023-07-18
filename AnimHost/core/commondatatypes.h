#ifndef COMMONDATATYPES_H
#define COMMONDATATYPES_H

#include <QObject>
#include <QString>
#include <QQuaternion>
#include <QMetaType>
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
public:
    Pose() {};
    ~Pose() {};
    Pose(const Pose&) {};
};

Q_DECLARE_METATYPE(Pose)



//!
//! \brief The prototype of a HumanoidBones class
//!
class HumanoidBones
{
public:
    HumanoidBones() { qDebug() << "HumanBones Hi!"; };
    HumanoidBones(const HumanoidBones& t) {};
    ~HumanoidBones() { qDebug() << "HumanBones Bye!!"; };
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
