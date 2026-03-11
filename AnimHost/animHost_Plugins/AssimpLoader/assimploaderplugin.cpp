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

 
#include "assimploaderplugin.h"
#include <iostream>

#include "assimphelper.h"
#include "animhosthelper.h"

#include <assimp/DefaultLogger.hpp>
#include <assimp/config.h>

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

	_validFrames = std::make_shared<AnimNodeData<ValidFrames>>();
	_validFrames->setData(std::make_shared<ValidFrames>());

	bDataValid = false;

	_pushButton = nullptr;
	widget = nullptr;
	_label = nullptr;
	_filePathLayout = nullptr;

}

QJsonObject AssimpLoaderPlugin::save() const
{
	QJsonObject nodeJson = NodeDelegateModel::save();

	nodeJson["dir"] = SourceDirectory;
	nodeJson["skeletonType"] = static_cast<int>(_skeletonType);
	nodeJson["sequencesFile"] = SequencesFilePath;
	nodeJson["sequencesOneIndexed"] = bSequencesOneIndexed;

	return nodeJson;
}

void AssimpLoaderPlugin::load(QJsonObject const& p)
{
	QJsonValue v = p["dir"];
	QJsonValue skeletonTypeVal = p["skeletonType"];
	QJsonValue sequencesFileVal = p["sequencesFile"];
	QJsonValue sequencesOneIndexedVal = p["sequencesOneIndexed"];

	if (!v.isUndefined()) {
		QString strDir = v.toString();

		if (!strDir.isEmpty()) {
			SourceDirectory = strDir;
			_folderSelect->SetDirectory(SourceDirectory);

			widget->adjustSize();
			widget->updateGeometry();
		}
	}

	if (!skeletonTypeVal.isUndefined()) {
		_skeletonType = static_cast<SkeletonType>(skeletonTypeVal.toInt());
		if (_skeletonTypeCombo) {
			_skeletonTypeCombo->setCurrentIndex(static_cast<int>(_skeletonType));
		}
	}

	if (!sequencesFileVal.isUndefined()) {
		SequencesFilePath = sequencesFileVal.toString();
		if (_sequencesFileSelect && !SequencesFilePath.isEmpty()) {
			_sequencesFileSelect->SetDirectory(SequencesFilePath);
		}
	}

	if (!sequencesOneIndexedVal.isUndefined()) {
		bSequencesOneIndexed = sequencesOneIndexedVal.toBool();
		if (_indexingCombo) {
			_indexingCombo->setCurrentIndex(bSequencesOneIndexed ? 1 : 0);
		}
	}
}

unsigned int AssimpLoaderPlugin::nDataPorts(QtNodes::PortType portType) const
{
    unsigned int result;

    if (portType == QtNodes::PortType::In)
        result = 0;
    else
        result = 3;  // Skeleton, Animation, ValidFrames

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
		case 2:
			return type = _validFrames->type();

		default:
			return type;
		}

}

std::shared_ptr<NodeData> AssimpLoaderPlugin::processOutData(QtNodes::PortIndex port)
{
	switch (port) {
	case 0:
		if (bDataValid)
			return _skeleton;
		break;
	case 1:
		if (bDataValid)
			return _animation;
		break;
	case 2:
		// ValidFrames is always available (empty if no Sequences.txt)
		return _validFrames;
	default:
		break;
	}
	return nullptr;
}

bool AssimpLoaderPlugin::isDataAvailable() {
	return true;
}

void AssimpLoaderPlugin::run() {

	// Parse Sequences.txt if configured
	parseSequencesFile();

	// Emit ValidFrames data update
	emitDataUpdate(2);

	//ToDo Move Processing
	QStringList files = loadFilesFromDir();

	// Reset the sequence counter for each run
	sequenceCounter = 1;
	for (auto file : files) {
		//QString file_name = QFileDialog::getOpenFileName(nullptr, "Import Animation", "C://", "(*.bvh *.fbx)");

		qDebug() << "Start processing " << file;

		if (SourceFilePath.compare(file) != 0) {

			Q_EMIT emitDataInvalidated(0);
			Q_EMIT emitDataInvalidated(1);


			SourceFilePath = file;
			bDataValid = false;

			QString shorty = AnimHostHelper::shortenFilePath(file, 10);

			/*_label->setText(shorty);
			_folderSelect->*/

			importAssimpData();

			// Apply sub-skeleton filtering based on selected skeleton type.
			// This removes FBX scene hierarchy nodes (like filename and "RootNode")
			// and keeps only actual skeleton bones.
			const auto& config = getSubSkeletonConfig(_skeletonType);
			UseSubSkeleton(config.rootBone, config.leafBones);
			if (config.applyChangeOfBasis) {
				_animation->getData()->ApplyChangeOfBasis();
			}

			emitDataUpdate(0);
			emitDataUpdate(1);

			qDebug() << "Processing " << shorty << " done.";

			Q_EMIT embeddedWidgetSizeUpdated();
		}
		emitRunNextNode();
	}
}

QWidget* AssimpLoaderPlugin::embeddedWidget()
{
	if (!widget) {

		widget = new QWidget();

		_folderSelect = new FolderSelectionWidget(widget);

		// Skeleton type selector for sub-skeleton filtering
		_skeletonTypeCombo = new QComboBox(widget);
		_skeletonTypeCombo->addItem("Bipedal (Survivor)", static_cast<int>(SkeletonType::Bipedal));
		_skeletonTypeCombo->addItem("Quadrupedal (MANN Dog)", static_cast<int>(SkeletonType::Quadrupedal));
		_skeletonTypeCombo->setCurrentIndex(static_cast<int>(_skeletonType));

		// Sequences.txt file selector (optional)
		_sequencesFileSelect = new FolderSelectionWidget(
			widget,
			FolderSelectionWidget::SelectionType::File,
			".txt",
			"Sequences files (*.txt);;All files (*)"
		);

		// Indexing mode selector
		_indexingCombo = new QComboBox(widget);
		_indexingCombo->addItem("0-indexed", 0);
		_indexingCombo->addItem("1-indexed", 1);
		_indexingCombo->setCurrentIndex(bSequencesOneIndexed ? 1 : 0);

		QVBoxLayout* layout = new QVBoxLayout();

		layout->addWidget(_folderSelect);
		layout->addWidget(new QLabel("Skeleton Type:"));
		layout->addWidget(_skeletonTypeCombo);
		layout->addWidget(new QLabel("Sequences File (optional):"));
		layout->addWidget(_sequencesFileSelect);
		layout->addWidget(new QLabel("Sequences Indexing:"));
		layout->addWidget(_indexingCombo);
		widget->setLayout(layout);

		connect(_folderSelect, &FolderSelectionWidget::directoryChanged, this, &AssimpLoaderPlugin::onFolderSelectionChanged);
		connect(_skeletonTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssimpLoaderPlugin::onSkeletonTypeChanged);
		connect(_sequencesFileSelect, &FolderSelectionWidget::directoryChanged, this, &AssimpLoaderPlugin::onSequencesFileChanged);
		connect(_indexingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssimpLoaderPlugin::onIndexingChanged);

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
	widget->updateGeometry();

	Q_EMIT embeddedWidgetSizeUpdated();
}

void AssimpLoaderPlugin::onSkeletonTypeChanged(int index)
{
	SkeletonType newType = static_cast<SkeletonType>(index);
	if (_skeletonType != newType) {
		_skeletonType = newType;
		const auto& config = getSubSkeletonConfig(_skeletonType);
		qDebug() << "Skeleton type changed to:" << (index == 0 ? "Bipedal" : "Quadrupedal");
		qDebug() << "  Root bone:" << config.rootBone;
		qDebug() << "  Leaf bones:" << config.leafBones.size();
		qDebug() << "  Apply change of basis:" << config.applyChangeOfBasis;
	}
}

void AssimpLoaderPlugin::loadAnimationData(aiAnimation* pASSIMPAnimation, Skeleton* pSkeleton, Animation* pAnimation, aiNode* pNode)
{
	pAnimation->mBones = std::vector<Bone>(pSkeleton->mNumBones, Bone());
	for (auto var : pSkeleton->bone_names)
	{
		pAnimation->mBones[var.second].mName = var.first;
	}

	AssimpHelper::setAnimationRestingPositionFromAssimpNode(*pNode, *pSkeleton, pAnimation);

	// Calculate mDurationFrames from actual keyframe count instead of mDuration
	int maxKeyframes = 0;
	for (int idx = 0; idx < pASSIMPAnimation->mNumChannels; idx++)
	{

		auto channel = pASSIMPAnimation->mChannels[idx];
		std::string name = channel->mNodeName.C_Str();
		int boneIndex = -1;
		try {
			boneIndex = pSkeleton->bone_names.at(name);
		}
		catch(const std::out_of_range& e){
			qWarning() << "Bone: " << name << "not found.";
			continue;
		}


		int numKeysRot = channel->mNumRotationKeys;
		int numKeysPos = channel->mNumPositionKeys;
		int numKeysScl = channel->mNumScalingKeys;

		// Track maximum keyframe count across all channels
		maxKeyframes = std::max(maxKeyframes, numKeysRot);
		maxKeyframes = std::max(maxKeyframes, numKeysPos);
		maxKeyframes = std::max(maxKeyframes, numKeysScl);

		// Log first channel's keyframe counts to see actual frame count
		if (idx == 0) {
			qDebug() << "[AssimpLoader] First channel actual keyframe counts:"
			         << "mNumRotationKeys:" << numKeysRot
			         << "mNumPositionKeys:" << numKeysPos
			         << "mNumScalingKeys:" << numKeysScl;
		}

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
	pAnimation->mDurationFrames = maxKeyframes;

	qDebug() << "[AssimpLoader] mDurationFrames set from keyframe count:" << pAnimation->mDurationFrames
	         << "(ASSIMP mDuration was:" << pASSIMPAnimation->mDuration << ")"
	         << "- Difference:" << (pAnimation->mDurationFrames - (int)pASSIMPAnimation->mDuration);
}


//takes the loaded skeleton and animation and creates a sub skeleton from the root bone and the leave bones, loaded animation gets updated
void AssimpLoaderPlugin::UseSubSkeleton(std::string pRootBone, std::vector<std::string> pLeaveBones) {

	auto oAnimation = _animation->getData();

	auto subSkel = _skeleton->getData()->CreateSubSkeleton(pRootBone, pLeaveBones);

	/*
	// print each bone in skeleton before and after sub skeleton
	qDebug() << "Bone Names before Subskeleton: ";
	for (auto var : _skeleton->getData()->bone_names)
	{
		qDebug() << var.first.c_str() << " :: " << var.second;
	}

	qDebug() << "Bone Names after Subskeleton: ";
	for (auto var : subSkel.bone_names)
	{
		qDebug() << var.first.c_str() << " :: " << var.second;
	}
	*/

	std::shared_ptr<Animation> anim = std::make_shared<Animation>();

	anim->mDurationFrames = oAnimation->mDurationFrames;
	anim->mDuration = oAnimation->mDuration;
	qDebug() << "[AssimpLoader] SubSkeleton mDurationFrames:" << anim->mDurationFrames
	         << "mDuration:" << anim->mDuration;
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
	importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0f);

	// Create a logger instance
	Assimp::DefaultLogger::create("", Assimp::Logger::DEBUGGING);

	// Now I am ready for logging my stuff
	Assimp::DefaultLogger::get()->info("this is my info-call");


	const unsigned int severity =  Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn;
	//const unsigned int severity =  Assimp::Logger::Err | Assimp::Logger::Warn;
	Assimp::DefaultLogger::get()->attachStream(new AssimpQTStream, severity);

	const  aiScene* scene = importer.ReadFile(SourceFilePath.toStdString(),
		aiProcess_SortByPType |
		aiProcess_ValidateDataStructure |  // Validate imported data integrity
		aiProcess_PopulateArmatureData |   // Ensure bone/animation data is complete
		aiProcess_GlobalScale);            // Apply AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY to positions

	auto keys = scene->mMetaData->mKeys;

	// Print all metadata
	/*for (int i = 0; i < scene->mMetaData->mNumProperties; i++)
	{
		qDebug() << scene->mMetaData->mKeys[i].C_Str() << " :: " << *static_cast<int32_t*>(scene->mMetaData->mValues[i].mData) ;
	}*/

	if (nullptr == scene) {
		qWarning() << "Loading failed: " << importer.GetErrorString();
		bDataValid = false;
		return;
	}

	if (scene->HasAnimations()) {
		
		QFileInfo fi(SourceFilePath);
		_animation->getData()->sourceName = fi.fileName();
		_animation->getData()->dataSetID = QUuid::createUuid().toString();
		_animation->getData()->sequenceID = sequenceCounter;

		sequenceCounter++;

		AssimpHelper::buildSkeletonFormAssimpNode(_skeleton->getData().get(), scene->mRootNode);

		std::shared_ptr<Skeleton> skel = _skeleton->getData();

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

void AssimpLoaderPlugin::onSequencesFileChanged()
{
	SequencesFilePath = _sequencesFileSelect->GetSelectedDirectory();

	widget->adjustSize();
	widget->updateGeometry();

	Q_EMIT embeddedWidgetSizeUpdated();

	qDebug() << "Sequences file changed to:" << SequencesFilePath;
}

void AssimpLoaderPlugin::onIndexingChanged(int index)
{
	bSequencesOneIndexed = (index == 1);
	qDebug() << "Sequences indexing changed to:" << (bSequencesOneIndexed ? "1-indexed" : "0-indexed");
}

void AssimpLoaderPlugin::parseSequencesFile()
{
	// Reset ValidFrames
	auto validFrames = std::make_shared<ValidFrames>();

	if (SequencesFilePath.isEmpty()) {
		// No Sequences.txt file configured - empty ValidFrames means "process all frames"
		_validFrames->setData(validFrames);
		qDebug() << "[AssimpLoader] No Sequences.txt configured - processing all frames";
		return;
	}

	QFile file(SequencesFilePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "[AssimpLoader] Failed to open Sequences.txt:" << SequencesFilePath;
		_validFrames->setData(validFrames);
		return;
	}

	QTextStream in(&file);
	int lineCount = 0;
	int frameCount = 0;

	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (line.isEmpty()) continue;

		lineCount++;

		// Parse line: [unknown] [frame_number] [label] [filename_stem.bvh] [hash]
		// Example: 1 180 Standard D1_001_KAN01_001.bvh 331c4ff26421fd44898a5e67ddd07067
		QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

		if (parts.size() < 4) {
			qWarning() << "[AssimpLoader] Malformed line" << lineCount << "in Sequences.txt:" << line;
			continue;
		}

		bool ok;
		int frameNumber = parts[1].toInt(&ok);
		if (!ok) {
			qWarning() << "[AssimpLoader] Invalid frame number on line" << lineCount << ":" << parts[1];
			continue;
		}

		// Convert to 0-indexed if source is 1-indexed
		if (bSequencesOneIndexed) {
			frameNumber -= 1;
		}

		QString filename = parts[3];
		QString stem = AnimHostHelper::extractFileStem(filename);

		// Add frame to the map
		validFrames->sequenceFrames[stem].push_back(frameNumber);
		frameCount++;
	}

	file.close();

	// Sort frame indices for each file (for binary_search in hasFrame)
	std::vector<QString> sequencesToRemove;
	for (auto& [stem, frames] : validFrames->sequenceFrames) {
		std::sort(frames.begin(), frames.end());
		// Remove duplicates
		frames.erase(std::unique(frames.begin(), frames.end()), frames.end());

		// Validate minimum sequence length for velocity filtering
		if (frames.size() < MIN_FRAMES_FOR_VELOCITY_FILTERING) {
			qWarning() << "[AssimpLoader] Sequence" << stem << "has only" << frames.size()
			           << "frames (minimum" << MIN_FRAMES_FOR_VELOCITY_FILTERING
			           << "required for Butterworth filtering) - skipping";
			sequencesToRemove.push_back(stem);
		}
	}

	// Remove sequences that are too short
	for (const auto& stem : sequencesToRemove) {
		validFrames->sequenceFrames.erase(stem);
	}

	_validFrames->setData(validFrames);

	qDebug() << "[AssimpLoader] Parsed Sequences.txt:" << frameCount << "frames across"
	         << validFrames->sequenceFrames.size() << "files (after filtering)";
}


