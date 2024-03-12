#include "commondatatypes.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
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
	restingRotation = o.restingRotation;

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
		return glm::quat(1,0,0,0);
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

glm::mat4 Bone::GetTransform(int frame) const {


	glm::quat outputRefJointRotation = GetOrientation(frame);
	//glm::quat invOutputRefJointRotation = glm::inverse(outputRefJointRotation);

	glm::quat orientation = GetOrientation(frame);
	glm::mat4 rotation = glm::toMat4(orientation);

	glm::vec3 scl = GetScale(frame);
	glm::mat4 scale = glm::scale(glm::mat4(1.0f), scl);

	glm::vec3 pos = GetPosition(frame);
	glm::mat4 translation(1.0f);
	translation = glm::translate(translation, pos);
	glm::mat4x4 TRS = translation * rotation * scale;

	return TRS;

};

//! Fills in the gaps between the ControlPoints by sampling new ControlPoints for every frame required by the ControlPath
void ControlPath::CreateSpline() {

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

