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

 
#ifndef ASSIMPHELPER_H
#define ASSIMPHELPER_H


#include<assimp/matrix4x4.h>
#include<assimp/quaternion.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/LogStream.hpp>
#include <commondatatypes.h>


class AssimpHelper
{

public:
	static inline glm::mat4 ConvertMatrixToGLM(const aiMatrix4x4& source) {

		glm::mat4 retMat;

		retMat[0][0] = source.a1;
		retMat[0][1] = source.b1;
		retMat[0][2] = source.c1;
		retMat[0][3] = source.d1;

		retMat[1][0] = source.a2;
		retMat[1][1] = source.b2;
		retMat[1][2] = source.c2;
		retMat[1][3] = source.d2;

		retMat[2][0] = source.a3;
		retMat[2][1] = source.b3;
		retMat[2][2] = source.c3;
		retMat[2][3] = source.d3;

		retMat[3][0] = source.a4;
		retMat[3][1] = source.b4;
		retMat[3][2] = source.c4;
		retMat[3][3] = source.d4;

		return retMat;
	}

	static inline glm::quat ConvertQuaternionToGLM(const aiQuaternion& source) {

		return glm::quat(source.w, source.x, source.y, source.z);
	}

	static inline glm::vec3 ConvertVectorToGLM(const aiVector3D& source) {

		return glm::vec3(source.x, source.y, source.z);
	}

	static void buildSkeletonFormAssimpNode(Skeleton* pSkeleton, aiNode* pNodes);
	static void indexSkeletonHirarchyFormAssimpNode(Skeleton* pSkeleton, aiNode* pNode, int* currentBoneCount);

	static void setAnimationRestingPositionFromAssimpNode(const aiNode& pNode, const Skeleton& pSkeleton, Animation* pAnimation);

};


class AssimpQTStream : public Assimp::LogStream {
public:
	// Write something using your own functionality
	void write(const char* message) {
		qDebug() << message;
	}
};


#endif