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

 
#include "animhosthelper.h"

#include <QFileInfo>
#include <QDir>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>

QString AnimHostHelper::shortenFilePath(const QString& filePath, int maxLength)
{
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    QString path = fileInfo.path();

    if (filePath.length() <= maxLength) {
        return filePath;
    }
    else {
        int ellipsisLen = 3;
        QString shortPath = "..."+path.right(maxLength - ellipsisLen);
        
        return  shortPath + QDir::separator() + fileName;
    }
    
    
    return QString();
}


void AnimHostHelper::ForwardKinematics(const Skeleton& skeleton, const Animation& animation, std::vector<glm::mat4>& outTransforms, int frame){
    
    
    outTransforms = std::vector<glm::mat4>(skeleton.mNumBones);


    std::function<void(glm::mat4, int)> buildTranforms;

    buildTranforms = [&](glm::mat4 currentT, int currentBone) {

        glm::quat orientation = animation.mBones[currentBone].GetOrientation(frame);
        glm::mat4 rotation = glm::toMat4(orientation);



        glm::vec3 scl = animation.mBones[currentBone].GetScale(frame);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), scl);


        glm::vec3 pos = animation.mBones[currentBone].GetPosition(frame);
        glm::mat4 translation(1.0f);

        translation = glm::translate(translation, pos);

        glm::mat4 local_transform = translation * rotation * scale;

        glm::mat4 globalT = currentT * local_transform;


        outTransforms[currentBone] = globalT;

        for (int i : skeleton.bone_hierarchy.at(currentBone)) {
            buildTranforms(globalT, i);
        }
    };

    int initcurrentBone = 0; //todo set specific root bone idx  s
    glm::mat4 initcurrentPos(1.0f);
    buildTranforms(initcurrentPos, initcurrentBone);

}

int AnimHostHelper::FindParentBone(const std::map<int, std::vector<int>>& bone_hierarchy, int currentBone)
{
    //search map values for current bone id and return the key/parent bone id

    for (auto const& [key, val] : bone_hierarchy) {
        for (int i : val) {
            if (i == currentBone) {
				return key;
			}
		}
	}

    // qDebug() << "FindParentBone: Parent bone not found";

    return -1;
}

glm::vec3 AnimHostHelper::ProjectPointOnGroundPlane(const glm::vec3& point, glm::vec3 groundNormal)
{
	//Project point on ground plane
	glm::vec3 groundPoint = point - glm::dot(point, groundNormal) * groundNormal;
    return groundPoint;
}

glm::mat4 AnimHostHelper::GetCoordinateSystemTransformationMatrix()
{
    glm::mat4 newmatrix(1.0f);

    newmatrix[0].x = 1.f;
    newmatrix[1].y = 0.f;
    newmatrix[1].z = -1.f;
    newmatrix[2].y = 1.f;
    newmatrix[2].z = 0.f;

    return newmatrix;
}
