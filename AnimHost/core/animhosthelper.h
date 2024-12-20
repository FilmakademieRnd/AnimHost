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