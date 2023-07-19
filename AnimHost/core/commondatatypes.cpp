#include "commondatatypes.h"

HumanoidBones::HumanoidBones(const HumanoidBones& t)
{
    // Spine bones
    spine = t.spine;
    chest = t.chest;
    upperChest= t.upperChest;
    head = t.head;
    leftShoulder = t.leftShoulder;
    leftUpperArm = t.leftUpperArm;
    leftLowerArm = t.leftLowerArm;
    leftHand = t.leftHand;
    rightShoulder = t.rightShoulder;
    rightUpperArm = t.rightUpperArm;
    rightLowerArm = t.rightLowerArm;
    rightHand = t.rightHand;
    leftUpperLeg = t.rightUpperLeg;
    leftLowerLeg = t.leftLowerLeg;
    leftFoot = t.leftFoot;
    leftToes = t.leftToes;
    rightUpperLeg = t.rightUpperLeg;
    rightLowerLeg = t.rightLowerLeg;
    rightFoot = t.rightFoot;
    rightToes = t.rightToes;
}
