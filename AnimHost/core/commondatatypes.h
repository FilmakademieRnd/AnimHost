#ifndef COMMONDATATYPES_H
#define COMMONDATATYPES_H

#include <QObject>
#include <QString>

class CommonDatatypes : public QObject
{
    Q_OBJECT
public:
    enum State{WAITING,RUNNING, DONE, ERROR};
    Q_ENUM(State)

    struct HumanoidBones {
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
};

#endif // COMMONDATATYPES_H
