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

        /*if (currentBone == 0) {
            translation = glm::translate(translation, glm::vec3(0.0, pos.y, 0.0));
        }
        else {
            translation = glm::translate(translation, pos);
        }*/

        translation = glm::translate(translation, pos);

        glm::mat4 local_transform = translation * rotation * scale;

        glm::mat4 globalT = currentT * local_transform;


        outTransforms[currentBone] = globalT;

        for (int i : skeleton.bone_hierarchy.at(currentBone)) {
            buildTranforms(globalT, i);
        }
    };

    int initcurrentBone = 0; //todo set specific root bone idx
    glm::mat4 initcurrentPos(1.0f);
    buildTranforms(initcurrentPos, initcurrentBone);

}
