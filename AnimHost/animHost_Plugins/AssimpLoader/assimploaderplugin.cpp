#include "assimploaderplugin.h"
#include <iostream>

#include "assimphelper.h"
#include "animhosthelper.h"


#include <QFileInfo>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>



AssimpLoaderPlugin::AssimpLoaderPlugin()
{
	_skeleton = std::make_shared<AnimNodeData<Skeleton>>();

	_animation = std::make_shared<AnimNodeData<Animation>>();

	bDataValid = false;

	_pushButton = nullptr;
	widget = nullptr;
	_label = nullptr;
	_filePathLayout = nullptr;
	
	qDebug() << this->name();
}

unsigned int AssimpLoaderPlugin::nPorts(QtNodes::PortType portType) const
{
    unsigned int result;

    if (portType == QtNodes::PortType::In)
        result = 0;
    else
        result = 2;

    return result;
}

NodeDataType AssimpLoaderPlugin::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        if (portIndex == 0)
            return type = _skeleton->type();
        else
            return type = _animation->type();
}

std::shared_ptr<NodeData> AssimpLoaderPlugin::outData(QtNodes::PortIndex port)
{
	if (!bDataValid) {
		importAssimpData();
	}
	
	if (bDataValid)
	{
		switch (port) {
		case 0:
			return _skeleton;
		case 1:
			return _animation;
		default:
			break;
		}
	}

	return nullptr;
}

QWidget* AssimpLoaderPlugin::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Import Animation");
		_label = new QLabel("Select Animation Path");

		_pushButton->resize(QSize(30, 30));
		_filePathLayout = new QHBoxLayout();

		_filePathLayout->addWidget(_label);
		_filePathLayout->addWidget(_pushButton);

		_filePathLayout->setSizeConstraint(QLayout::SetMinimumSize);

		widget = new QWidget();

		widget->setLayout(_filePathLayout);





		connect(_pushButton, &QPushButton::released, this, &AssimpLoaderPlugin::onButtonClicked);
	}

	widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
		"QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
		"QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
		"QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
	);

	return widget;
}

void AssimpLoaderPlugin::loadAnimationData(aiAnimation* pASSIMPAnimation, Skeleton* pSkeleton, Animation* pAnimation, aiNode* pNode)
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

void AssimpLoaderPlugin::importAssimpData()
{
	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	qDebug() << Q_FUNC_INFO;

	const  aiScene* scene = importer.ReadFile(SourceFilePath.toStdString(),
		aiProcess_SortByPType);

	if (nullptr == scene) {
		qDebug() << Q_FUNC_INFO << "\n" << importer.GetErrorString();
		bDataValid = false;
		return;
	}

	if (scene->HasAnimations()) {
		for (int anim_idx = 0; anim_idx < scene->mNumAnimations; anim_idx++) {
			//qDebug() << scene->mAnimations[anim_idx]->mName.C_Str() << "\n";
			unsigned int numChannels = scene->mAnimations[anim_idx]->mNumChannels;

			for (unsigned int ch_idx = 0; ch_idx < numChannels; ch_idx++) {
				// qDebug() << scene->mAnimations[anim_idx]->mChannels[ch_idx]->mNodeName.C_Str() << "\n";
			}
		}

		AssimpHelper::buildSkeletonFormAssimpNode(_skeleton->getData().get(), scene->mRootNode);
		loadAnimationData(scene->mAnimations[0], _skeleton->getData().get(), _animation->getData().get(), scene->mRootNode);

		bDataValid = true;
	}
}


void AssimpLoaderPlugin::onButtonClicked()
{
	qDebug() << "Clicked";
	QString file_name = QFileDialog::getOpenFileName(nullptr, "Import Animation", "C://", "(*.bvh *.fbx)");

	SourceFilePath = file_name;
	bDataValid = false;

	qDebug() << file_name;

	QString shorty =AnimHostHelper::shortenFilePath(file_name, 10);

	_label->setText(shorty);


	

	Q_EMIT dataUpdated(0);
	Q_EMIT dataUpdated(1);

	Q_EMIT embeddedWidgetSizeUpdated();

}




