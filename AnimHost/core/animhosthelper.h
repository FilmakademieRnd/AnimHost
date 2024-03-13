#ifndef ANIMHOSTHELPERS_H
#define ANIMHOSTHELPERS_H


#include<glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <commondatatypes.h>


class ANIMHOSTCORESHARED_EXPORT AnimHostHelper
{

public:
	static QString shortenFilePath(const QString& filePath, int maxLength);

	static void ForwardKinematics(const Skeleton& skeleton, const Animation& animation, std::vector<glm::mat4>& outTransforms, int frame);

	static int FindParentBone(const std::map<int, std::vector<int>>& bone_hierarchy, int currentBone);

};


#endif