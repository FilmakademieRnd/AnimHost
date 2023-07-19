#include "assimploadernode.h"
#include "assimphelper.h"

AssimpLoaderNode::AssimpLoaderNode()
{

	_skeleton = std::make_shared<SkeletonNodeData>();

	_animation = std::make_shared<AnimationNodeData>();
    
	bDataValid = false;
}


unsigned int AssimpLoaderNode::nPorts(PortType portType) const
{
    unsigned int result;

    if (portType == PortType::In)
        result = 0;
    else
        result = 2;

    return result;
}

NodeDataType AssimpLoaderNode::dataType(PortType portType, PortIndex index) const
{
    NodeDataType type;
    if (portType == PortType::In)
        return type;
    else
		if(index == 0)
			return type = _skeleton->type();
		else
			return type = _animation->type();

}

std::shared_ptr<NodeData> AssimpLoaderNode::outData(PortIndex index)
{
    //QVariant data = _plugin->outputs->at(index);
	if (!bDataValid) {
		importAssimpData();
		bDataValid = true;
	}
		

	switch (index) {
	case 0:
		return _skeleton;
	case 1:
		return _animation;
	default:
		break;
	}
	
	return nullptr;
	

}

void AssimpLoaderNode::loadAnimationData(aiAnimation* pASSIMPAnimation, Skeleton* pSkeleton, Animation* pAnimation, aiNode* pNode)
{
	pAnimation->mDurationFrames = pASSIMPAnimation->mDuration;


	pAnimation->mBones = std::vector<Bone>(pSkeleton->mNumBones, Bone());
	for (auto var : pSkeleton->bone_names)
	{
		pAnimation->mBones[var.second].mName = var.first;
	}

	AssimpHelper::setAnimationRestingPositionFromAssimpNode(*pNode, *pSkeleton, pAnimation);

	for (int idx = 0; idx < pASSIMPAnimation->mNumChannels; idx++)
	{

		auto channel = pASSIMPAnimation->mChannels[idx];
		std::string name = channel->mNodeName.C_Str();

		int boneIndex = pSkeleton->bone_names.at(name);

		int numKeysRot = channel->mNumRotationKeys;
		int numKeysPos = channel->mNumPositionKeys;
		int numKeysScl = channel->mNumScalingKeys;

		for (int i = 0; i < numKeysRot; i++)
		{
			pAnimation->mBones.at(boneIndex).mNumKeysRotation = numKeysRot;
			auto rotKey = channel->mRotationKeys[i];
			glm::quat orientation = AssimpHelper::ConvertQuaternionToGLM(rotKey.mValue);
			pAnimation->mBones.at(boneIndex).mRotationKeys.push_back({ (float)rotKey.mTime,orientation });
		}

		for (int i = 0; i < numKeysPos; i++)
		{
			pAnimation->mBones.at(boneIndex).mNumKeysPosition = numKeysPos;
			auto posKey = channel->mPositionKeys[i];
			glm::vec3 position = AssimpHelper::ConvertVectorToGLM(posKey.mValue);
			pAnimation->mBones.at(boneIndex).mPositonKeys.push_back({ (float)posKey.mTime,position });
		}

		for (int i = 0; i < numKeysScl; i++)
		{
			pAnimation->mBones.at(boneIndex).mNumKeysScale = numKeysScl;
			auto sclKey = channel->mScalingKeys[i];
			glm::vec3 scale = AssimpHelper::ConvertVectorToGLM(sclKey.mValue);
			pAnimation->mBones.at(boneIndex).mScaleKeys.push_back({ (float)sclKey.mTime,scale });
		}
	}
}

void AssimpLoaderNode::importAssimpData()
{
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    const  aiScene* scene = importer.ReadFile(SourceFilePath.toStdString(),
        aiProcess_SortByPType);

    if (nullptr == scene) {
        qDebug() << Q_FUNC_INFO << "\n" << importer.GetErrorString();
        return;
    }

    if (scene->HasAnimations()) {
        for (int anim_idx = 0; anim_idx < scene->mNumAnimations; anim_idx++) {
            qDebug() << scene->mAnimations[anim_idx]->mName.C_Str() << "\n";
            unsigned int numChannels = scene->mAnimations[anim_idx]->mNumChannels;

            for (unsigned int ch_idx = 0; ch_idx < numChannels; ch_idx++) {
                qDebug() << scene->mAnimations[anim_idx]->mChannels[ch_idx]->mNodeName.C_Str() << "\n";
            }
        }

		AssimpHelper::buildSkeletonFormAssimpNode(_skeleton->skeleton(), scene->mRootNode);

		loadAnimationData(scene->mAnimations[0], _skeleton->skeleton(), _animation->animation(), scene->mRootNode);
    }
}


