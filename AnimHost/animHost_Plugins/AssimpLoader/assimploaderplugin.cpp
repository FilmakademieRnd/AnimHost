#include "assimploaderplugin.h"
#include <iostream>

#include "assimphelper.h"
#include "animhosthelper.h"

#include <assimp/DefaultLogger.hpp>


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

QJsonObject AssimpLoaderPlugin::save() const
{
	QJsonObject nodeJson = NodeDelegateModel::save();

	nodeJson["dir"] = SourceDirectory;

	return nodeJson;
}

void AssimpLoaderPlugin::load(QJsonObject const& p)
{
	QJsonValue v = p["dir"];

	if (!v.isUndefined()) {
		QString strDir = v.toString();

		if (!strDir.isEmpty()) {
			SourceDirectory = strDir;
		}
	}
}

unsigned int AssimpLoaderPlugin::nDataPorts(QtNodes::PortType portType) const
{
    unsigned int result;

    if (portType == QtNodes::PortType::In)
        result = 0;
    else
        result = 2;

    return result;
}

NodeDataType AssimpLoaderPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
		switch (portIndex) {
		case 0:
			return type = _skeleton->type();
		case 1:
			return type = _animation->type();

		default:
			return type;
		}
             
}

std::shared_ptr<NodeData> AssimpLoaderPlugin::processOutData(QtNodes::PortIndex port)
{

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

void AssimpLoaderPlugin::run() {
	
	//ToDo Move Processing
	QStringList files = loadFilesFromDir();

	for (auto file : files) {
		//QString file_name = QFileDialog::getOpenFileName(nullptr, "Import Animation", "C://", "(*.bvh *.fbx)");

		qDebug() << "... Start Process " << file;

		Q_EMIT emitDataInvalidated(0);
		Q_EMIT emitDataInvalidated(1);

		SourceFilePath = file;
		bDataValid = false;

		QString shorty = AnimHostHelper::shortenFilePath(file, 10);

		/*_label->setText(shorty);
		_folderSelect->*/

		importAssimpData();

		//Experimental.
		// If character is our own "survivor" character, we need to adjust the skeleton and animation data
		if (bIsSurvivorChar)
		{
			UseSubSkeleton("hip", { "hand_R", "hand_L" });
			_animation->getData()->ApplyChangeOfBasis();
		}

		emitDataUpdate(0);
		emitDataUpdate(1);

		qDebug() << "Process " << shorty << "... Done";

		Q_EMIT embeddedWidgetSizeUpdated();

		emitRunNextNode();
	}
}

QWidget* AssimpLoaderPlugin::embeddedWidget()
{
	if (!widget) {

		widget = new QWidget();

		_folderSelect = new FolderSelectionWidget(widget);


		QVBoxLayout* layout = new QVBoxLayout();

		layout->addWidget(_folderSelect);
		widget->setLayout(layout);

		connect(_folderSelect, &FolderSelectionWidget::directoryChanged, this, &AssimpLoaderPlugin::onFolderSelectionChanged);

	}

	widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
		"QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
		"QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
		"QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
	);

	return widget;
}

void AssimpLoaderPlugin::onFolderSelectionChanged()
{
	SourceDirectory = _folderSelect->GetSelectedDirectory() + "/";

	widget->adjustSize();
	//widget->parentWidget()->adjustSize();
	widget->updateGeometry();

	Q_EMIT embeddedWidgetSizeUpdated();
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
		int boneIndex = -1;
		try {
			boneIndex = pSkeleton->bone_names.at(name);
		}
		catch(const std::out_of_range& e){
			qDebug() << "Bone: " << name << "not found.";
			continue;
		}
		

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


//takes the loaded skeleton and animation and creates a sub skeleton from the root bone and the leave bones, loaded animation gets updated
void AssimpLoaderPlugin::UseSubSkeleton(std::string pRootBone, std::vector<std::string> pLeaveBones) {


	auto oAnimation = _animation->getData();

	auto subSkel = _skeleton->getData()->CreateSubSkeleton(pRootBone, pLeaveBones);

	//print each bone in skeleton bone_names
	qDebug() << "Bone Names before Subskeleton: ";
	for (auto var : _skeleton->getData()->bone_names)
	{
		qDebug() << var.first.c_str() << " :: " << var.second;
	}

	//auto subSkel = skel->CreateSubSkeleton("hip", { "hand_R", "hand_L" });
	qDebug() << "Bone Names after Subskeleton: ";
	for (auto var : subSkel.bone_names)
	{
		qDebug() << var.first.c_str() << " :: " << var.second;
	}


	std::shared_ptr<Animation> anim = std::make_shared<Animation>();

	anim->mDurationFrames = oAnimation->mDurationFrames;
	anim->mDuration = oAnimation->mDuration;
	anim->sequenceID = oAnimation->sequenceID;
	anim->sourceName = oAnimation->sourceName;
	anim->dataSetID = oAnimation->dataSetID;

	anim->mBones = std::vector<Bone>(subSkel.mNumBones, Bone());

	int newIdxCounter = 0;

	for (auto idx : subSkel) {
		anim->mBones[newIdxCounter] = oAnimation->mBones[idx];
		newIdxCounter++;
	}



	//Create copy of SubSkeleton and reset the index of the bones in skeleton
	Skeleton subSkelCopy = subSkel;

	subSkelCopy.bone_names.clear();
	subSkelCopy.bone_names_reverse.clear();
	subSkelCopy.bone_hierarchy.clear();

	subSkelCopy.rootBoneID = 0;

	newIdxCounter = 0;

	for (auto idx : subSkel) {
		subSkelCopy.bone_names[subSkel.bone_names_reverse[idx]] = newIdxCounter;
		subSkelCopy.bone_names_reverse[newIdxCounter] = subSkel.bone_names_reverse[idx];
		newIdxCounter++;
	}


	//update bone hirarchy to match with new bone index
	for (auto workingBoneIdx : subSkel) {
		
		std::string workingBoneName = subSkel.bone_names_reverse[workingBoneIdx];
		int parendIdx = subSkelCopy.bone_names[workingBoneName];

		subSkelCopy.bone_hierarchy[parendIdx] = std::vector<int>();

		for(auto childIdx : subSkel.bone_hierarchy[workingBoneIdx]) {
			std::string childName = subSkel.bone_names_reverse[childIdx];
			subSkelCopy.bone_hierarchy[parendIdx].push_back(subSkelCopy.bone_names[childName]);
		}

	}

	_animation->setData(anim);
	_skeleton->setData(std::make_shared<Skeleton>(subSkelCopy));

}


void AssimpLoaderPlugin::importAssimpData()
{
	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);


	// Create a logger instance
	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);

	// Now I am ready for logging my stuff
	Assimp::DefaultLogger::get()->info("this is my info-call");


	const unsigned int severity =  Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn;
	Assimp::DefaultLogger::get()->attachStream(new AssimpQTStream, severity);

	const  aiScene* scene = importer.ReadFile(SourceFilePath.toStdString(),
		aiProcess_SortByPType);

	auto keys = scene->mMetaData->mKeys;

	for (int i = 0; i < scene->mMetaData->mNumProperties; i++)
	{
		qDebug() << scene->mMetaData->mKeys[i].C_Str() << " :: " << *static_cast<int32_t*>(scene->mMetaData->mValues[i].mData) ;
	}

	if (nullptr == scene) {
		qDebug() << Q_FUNC_INFO << "\n" << importer.GetErrorString();
		bDataValid = false;
		return;
	}

	if (scene->HasAnimations()) {
		
		QFileInfo fi(SourceFilePath);
		_animation->getData()->sourceName = fi.fileName();
		_animation->getData()->dataSetID = QUuid::createUuid().toString();
		_animation->getData()->sequenceID = sequenceCounter;

		sequenceCounter++;
		//_animation->getData()->uuId = QUuid::createUuid();

		AssimpHelper::buildSkeletonFormAssimpNode(_skeleton->getData().get(), scene->mRootNode);

		std::shared_ptr<Skeleton> skel = _skeleton->getData();

		//print each bone in skeleton bone_names
		//qDebug() << "Bone Names before Subskeleton: ";
		//for (auto var : skel->bone_names)
		//{
		//	qDebug() << var.first.c_str() << " :: " << var.second;
		//}

		//auto subSkel = skel->CreateSubSkeleton("hip", { "hand_R", "hand_L" });
		//qDebug() << "Bone Names after Subskeleton: ";
		//for (auto var : subSkel.bone_names)
		//{
		//	qDebug() << var.first.c_str() << " :: " << var.second;
		//}

		////test the skeleton iterator
		//for(auto it= subSkel.begin(); it != subSkel.end(); ++it)
		//{
		//	int bone = *it;
		//	qDebug() << subSkel.bone_names_reverse[bone] << "::" << bone;
		//}

		//for(auto it: subSkel){
		//	qDebug() << subSkel.bone_names_reverse[it] << "::" << it;
		//}

		//skel->bone_names

		loadAnimationData(scene->mAnimations[0], _skeleton->getData().get(), _animation->getData().get(), scene->mRootNode);

		bDataValid = true;
	}
}


QStringList AssimpLoaderPlugin::loadFilesFromDir()
{
	if (!SourceDirectory.isEmpty()) {

		QString directory = SourceDirectory;

		QStringList filter = { "*.bvh","*.fbx","*.glb"};


		QDirIterator itr(directory, filter, QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
		
		QStringList files;

		while (itr.hasNext())
			files << itr.next();

		files.sort();

		return files;
	}

	return {""};
}



