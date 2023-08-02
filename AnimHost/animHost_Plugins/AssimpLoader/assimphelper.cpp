#include "assimphelper.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void AssimpHelper::buildSkeletonFormAssimpNode(Skeleton* pSkeleton, aiNode* pNodes)
{
	//Build Hirarchy
	int boneIndexCount = 0;
	AssimpHelper::indexSkeletonHirarchyFormAssimpNode(pSkeleton, pNodes, &boneIndexCount);

	for (auto itr = pSkeleton->bone_names.begin(); itr != pSkeleton->bone_names.end(); itr++) {
		pSkeleton->bone_names_reverse[itr->second] = itr->first;
	}
	pSkeleton->mNumBones = boneIndexCount + 1;
}

void AssimpHelper::indexSkeletonHirarchyFormAssimpNode(Skeleton* pSkeleton, aiNode* pNode, int* currentBoneCount)
{
	int currentBoneIdx = *currentBoneCount;
	pSkeleton->bone_names[pNode->mName.C_Str()] = currentBoneIdx;


	pSkeleton->bone_hierarchy[currentBoneIdx] = std::vector<int>(pNode->mNumChildren);

	for (int child = 0; child < pNode->mNumChildren; child++) {

		auto child_node = pNode->mChildren[child];
		*currentBoneCount += 1;

		pSkeleton->bone_hierarchy[currentBoneIdx][child] = *currentBoneCount;
		AssimpHelper::indexSkeletonHirarchyFormAssimpNode(pSkeleton,child_node, currentBoneCount);
	}
}

void AssimpHelper::setAnimationRestingPositionFromAssimpNode(const aiNode& pNode, const Skeleton& pSkeleton, Animation* pAnimation)
{
	auto name = pNode.mName.C_Str();

	int bone_idx = pSkeleton.bone_names.at(name);


	pAnimation->mBones[bone_idx].mRestingTransform = AssimpHelper::ConvertMatrixToGLM(pNode.mTransformation);



	for (int child = 0; child < pNode.mNumChildren; child++) {

		auto child_node = pNode.mChildren[child];
		AssimpHelper::setAnimationRestingPositionFromAssimpNode(*child_node,pSkeleton,pAnimation);
	}
}
