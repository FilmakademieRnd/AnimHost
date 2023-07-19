#include "commondatatypes.h"

//#include "AssimpHelper.h"

Bone::Bone(std::string name, int id, int numPos, int numRot, int numScl, glm::mat4 rest)
{
	mName = name;
	mID = id;
	mNumKeysPosition = numPos;
	mNumKeysRotation = numRot;
	mNumKeysScale = numScl;

	mRestingTransform = rest;
}

Bone::Bone()
{
	mName = "";
	mNumKeysPosition = 0;
	mNumKeysRotation = 0;
	mNumKeysScale = 0;

	mRestingTransform = glm::mat4(1.0f);
}

glm::quat Bone::GetOrientation(int frame)
{
	if (mRotationKeys.size() == 0)
	{
		return glm::quat();
	}
	else
	{
		if (frame < mRotationKeys.size()) {
			return mRotationKeys[frame].orientation;
		}
		else {
			return mRotationKeys[mRotationKeys.size() - 1].orientation;
		}
	}

}

glm::vec3 Bone::GetPosition(int frame)
{
	if (mPositonKeys.size() == 0)
	{
		return glm::vec3(0.0);
	}
	else
	{

		if (frame < mPositonKeys.size()) {
			return mPositonKeys[frame].position;
		}
		else {
			return mPositonKeys[mPositonKeys.size() - 1].position;
		}

	}

}

glm::vec3 Bone::GetScale(int frame)
{
	if (mScaleKeys.size() == 0)
	{
		return glm::vec3(1.0);
	}
	else
	{
		if (frame < mScaleKeys.size()) {
			return mScaleKeys[frame].scale;
		}
		else {
			return mScaleKeys[mScaleKeys.size() - 1].scale;
		}
	}

}

//void Animation::SetRestingPosition(const aiNode& pNode, const Skeleton& pSkeleton)
//{
//	auto name = pNode.mName.C_Str();
//
//	int bone_idx = pSkeleton.bone_names.at(name);
//
//
//	this->mBones[bone_idx].mRestingTransform = AssimpHelper::ConvertMatrixToGLM(pNode.mTransformation);
//
//
//
//	for (int child = 0; child < pNode.mNumChildren; child++) {
//
//		auto child_node = pNode.mChildren[child];
//		this->SetRestingPosition(*child_node, pSkeleton);
//	}
//}


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
