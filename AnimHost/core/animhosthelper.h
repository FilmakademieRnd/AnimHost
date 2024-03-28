#ifndef ANIMHOSTHELPERS_H
#define ANIMHOSTHELPERS_H


#include<glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <commondatatypes.h>


class ANIMHOSTCORESHARED_EXPORT AnimHostHelper
{


public:


public:
	static QString shortenFilePath(const QString& filePath, int maxLength);

	static void ForwardKinematics(const Skeleton& skeleton, const Animation& animation, std::vector<glm::mat4>& outTransforms, int frame);

	static int FindParentBone(const std::map<int, std::vector<int>>& bone_hierarchy, int currentBone);


	/*
	* @brief: This function returns Coordinate system transformation matrix swapping the Y and Z axis and negating the new Z axis
	*/
	static glm::mat4 GetCoordinateSystemTransformationMatrix();

};


#endif