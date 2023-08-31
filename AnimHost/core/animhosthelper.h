#ifndef ANIMHOSTHELPERS_H
#define ANIMHOSTHELPERS_H


#include<glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <commondatatypes.h>


class ANIMHOSTCORESHARED_EXPORT AnimHostHelper
{

public:
	static QString shortenFilePath(const QString& filePath, int maxLength);

};


#endif