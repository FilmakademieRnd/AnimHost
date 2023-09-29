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

Bone::Bone(const Bone& o, int frame)
{
	mName = o.mName;
	mID = o.mID;
	mNumKeysPosition = 1;
	mNumKeysRotation = 1;
	mNumKeysScale = 1;

	mRestingTransform = o.mRestingTransform;

	this->mPositonKeys.push_back({ 0.0,  o.GetPosition(frame)});
	this->mRotationKeys.push_back({ 0.0,  o.GetOrientation(frame)});
	this->mScaleKeys.push_back({ 0.0, o.GetScale(frame)});

}

Bone::Bone()
{
	mName = "";
	mNumKeysPosition = 0;
	mNumKeysRotation = 0;
	mNumKeysScale = 0;

	mRestingTransform = glm::mat4(1.0f);
}

glm::quat Bone::GetOrientation(int frame) const
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

glm::vec3 Bone::GetPosition(int frame) const
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

glm::vec3 Bone::GetScale(int frame) const
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

