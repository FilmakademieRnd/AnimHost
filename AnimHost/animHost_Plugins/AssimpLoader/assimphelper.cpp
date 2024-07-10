/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *    
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *    
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program; 
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */

 
#include "assimphelper.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>

#include <glm/gtx/quaternion.hpp>

#include <glm/gtc/quaternion.hpp>

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


	pSkeleton->bone_hierarchy[currentBoneIdx] = std::vector<int>(pNode->mNumChildren, -1);

	for (int child = 0; child < pNode->mNumChildren; child++) {

		auto child_node = pNode->mChildren[child];
		*currentBoneCount += 1;

		pSkeleton->bone_hierarchy[currentBoneIdx][child] = *currentBoneCount;
		AssimpHelper::indexSkeletonHirarchyFormAssimpNode(pSkeleton, child_node, currentBoneCount);
	}

	
}

void AssimpHelper::setAnimationRestingPositionFromAssimpNode(const aiNode& pNode, const Skeleton& pSkeleton, Animation* pAnimation)
{
	auto name = pNode.mName.C_Str();

	int bone_idx = pSkeleton.bone_names.at(name);

	aiVector3D scale;
	aiQuaternion quat;
	aiVector3D pos;

	pNode.mTransformation.Decompose(scale, quat, pos);

	pAnimation->mBones[bone_idx].mRestingTransform = AssimpHelper::ConvertMatrixToGLM(pNode.mTransformation);

	pAnimation->mBones[bone_idx].restingRotation = AssimpHelper::ConvertQuaternionToGLM(quat);


	for (int child = 0; child < pNode.mNumChildren; child++) {

		auto child_node = pNode.mChildren[child];
		AssimpHelper::setAnimationRestingPositionFromAssimpNode(*child_node,pSkeleton,pAnimation);
	}
}
