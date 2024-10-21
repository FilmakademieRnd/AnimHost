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

 

#define GLM_FORCE_SWIZZLE
#include "ModeAdaptivePreprocessPlugin.h"

#include <QElapsedTimer>
#include <FileHandler.h>
#include <FrameRange.h>
#include <MathUtils.h>


#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <commondatatypes.h>
#include <animhosthelper.h>


ModeAdaptivePreprocessPlugin::ModeAdaptivePreprocessPlugin()
{
}

ModeAdaptivePreprocessPlugin::~ModeAdaptivePreprocessPlugin()
{
}


unsigned int ModeAdaptivePreprocessPlugin::nDataPorts(QtNodes::PortType portType) const
{
	if (portType == QtNodes::PortType::In)
		return 4;
	else
		return 0;
}

NodeDataType ModeAdaptivePreprocessPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
	NodeDataType type;

	if (portType == QtNodes::PortType::In){
		switch (portIndex) {
		case 0:
			return AnimNodeData<Skeleton>::staticType();
		case 1:
			return AnimNodeData<PoseSequence>::staticType();
		case 2:
			return AnimNodeData<JointVelocitySequence>::staticType();
		case 3:
			return AnimNodeData<Animation>::staticType();
		default:
			break;
		}
	}
	
	return type;
}

QJsonObject ModeAdaptivePreprocessPlugin::save() const 
{
	QJsonObject nodeJson = NodeDelegateModel::save();

	nodeJson["dir"] = exportDirectory;

	return nodeJson;
}

void ModeAdaptivePreprocessPlugin::load(QJsonObject const& p)
{
	
	QJsonValue v = p["dir"];

	if (!v.isUndefined()) {
		QString strDir = v.toString();

		if (!strDir.isEmpty()) {
			exportDirectory = strDir;
			_folderSelect->SetDirectory(exportDirectory);

			_widget->adjustSize();
			_widget->updateGeometry();
		}
	}

}


void ModeAdaptivePreprocessPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
	qDebug() << "ModeAdaptivePreprocessPlugin setInData";

	if (!data) {
		switch (portIndex) {
		case 0:
			_skeletonIn.reset();
			break;
		case 1:
			_poseSequenceIn.reset();
			break;
		case 2:
			_jointVelocitySequenceIn.reset();
			break;
		case 3:
			_animationIn.reset();

		default:
			return;
		}
		return;
	}


	switch (portIndex) {
	case 0:
		_skeletonIn = std::static_pointer_cast<AnimNodeData<Skeleton>>(data);
		_boneSelect->UpdateBoneSelection(*(_skeletonIn.lock()->getData().get()));
		Q_EMIT embeddedWidgetSizeUpdated();
		break;
	case 1:
		_poseSequenceIn = std::static_pointer_cast<AnimNodeData<PoseSequence>>(data);
		break;
	case 2:
		_jointVelocitySequenceIn = std::static_pointer_cast<AnimNodeData<JointVelocitySequence>>(data);
		break;
	case 3:
		_animationIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
		break;
	default:
		return;
	}

}

bool ModeAdaptivePreprocessPlugin::isDataAvailable() {
	return _poseSequenceIn.lock() && _animationIn.lock() && _jointVelocitySequenceIn.lock() && _skeletonIn.lock();
}

void ModeAdaptivePreprocessPlugin::run()
{

	if (!isDataAvailable()) {
		qDebug() << "ModeAdaptivePreprocessPlugin: Incomplete Input Data";
		return;
	}
	else {
		auto sp_poseSeq = _poseSequenceIn.lock();
		auto poseSequenceIn = sp_poseSeq->getData();

		auto sp_animation = _animationIn.lock();
		auto animation = sp_animation->getData();

		auto sp_velSeq = _jointVelocitySequenceIn.lock();
		auto velSeq = sp_velSeq->getData();

		auto sp_skeleton = _skeletonIn.lock();
		auto skeleton = sp_skeleton->getData();


		qDebug() << "ModeAdaptivePreprocessPlugin: Input Data Complete";

		rootBoneTransforms.clear();

		rootSequenceData.clear();
		sequenceRelativeJointPosition.clear();
		sequenceRelativeJointVelocities.clear();
		sequenceRelativJointRotations.clear();
		sequenceRelativJointRotations6D.clear();

		Y_SequenceDeltaUpdate.clear();
		Y_RootSequenceData.clear();
		Y_SequenceRelativeJointPosition.clear();
		Y_SequenceRelativeJointVelocities.clear();
		Y_SequenceRelativJointRotations.clear();
		Y_SequenceRelativJointRotations6D.clear();



		//Preprocess Root Transform for Biped once for all frames
		rootBoneTransforms = prepareBipedRoot(poseSequenceIn, skeleton);


		//Offset to start of sequence, allows for enough frames to be left for past trajectory samples
		int start = 60;

		//Offset to last frame of sequence, allows for enough frames to be left for trajectory 
		//and output (trajectory of next frame)
		int end = animation->mDurationFrames - 60;

		for (int frameCounter = start; frameCounter <= end; frameCounter++) {
			processFrame(frameCounter, poseSequenceIn, animation, velSeq, skeleton);
		}

		// Clear existing data
		clearExistingData();

		// Write Data to Files
		writeMetaData();
		writeInputData();
		writeOutputData();

	}
}

void ModeAdaptivePreprocessPlugin::processFrame(int frameCounter, std::shared_ptr<PoseSequence> poseSequenceIn, std::shared_ptr<Animation> animation, std::shared_ptr<JointVelocitySequence> velSeq, std::shared_ptr<Skeleton> skeleton)
{
	int referenceFrame = frameCounter;

	// ==============================
	// INPUT SECTION
	// ==============================

	//glm::mat4 rootTransform = animation->CalculateRootTransform(referenceFrame, rootbone_idx);
	glm::mat4 rootTransform = rootBoneTransforms[referenceFrame];

	//Root Trajectory
	std::vector<float> flatTrajectoryData = prepareTrajectoryData(referenceFrame + 1, animation, rootTransform, false);
	rootSequenceData.push_back(flatTrajectoryData);

	//Current joint positions relative to root position.
	std::vector<glm::vec3> relativeJointPosition = prepareJointPositions(referenceFrame, poseSequenceIn, rootTransform);
	sequenceRelativeJointPosition.push_back(relativeJointPosition);

	// Joint Rotations
	std::vector<Rotation6D> relativeJointRotations6D = prepareJointRotations6D(referenceFrame, animation, skeleton, rootTransform, false);
	sequenceRelativJointRotations6D.push_back(relativeJointRotations6D);

	// Joint Velocity
	std::vector<glm::vec3> relativeJointVelocities = prepareJointVelocities(referenceFrame, velSeq, rootTransform);
	sequenceRelativeJointVelocities.push_back(relativeJointVelocities);

 	// ==============================
	// OUTPUT SECTION
	// ==============================

	//glm::mat4 nextRootTransform = animation->CalculateRootTransform(referenceFrame + 1, rootbone_idx);
	glm::mat4 nextRootTransform = rootBoneTransforms[referenceFrame + 1];
	
	//Output Root Trajectory
	std::vector<float> outFlatTrajectoryData = prepareTrajectoryData(referenceFrame, animation, nextRootTransform,true);
	Y_RootSequenceData.push_back(outFlatTrajectoryData);

	//Output Joint Positions
	std::vector<glm::vec3> OutputJointPosition = prepareJointPositions(referenceFrame, poseSequenceIn, nextRootTransform, true);
	Y_SequenceRelativeJointPosition.push_back(OutputJointPosition);

	//Output Joint Rotations
	std::vector<Rotation6D> OutputJointRotations6D = prepareJointRotations6D(referenceFrame, animation, skeleton, nextRootTransform, true);
	Y_SequenceRelativJointRotations6D.push_back(OutputJointRotations6D);

	//Output Joint Velocities
	std::vector<glm::vec3> OutputJointVelocities = prepareJointVelocities(referenceFrame, velSeq, nextRootTransform);
	Y_SequenceRelativeJointVelocities.push_back(OutputJointVelocities);

	// Calculate root delta update
	glm::vec2 deltaForward = MathUtils::ForwardTo(nextRootTransform, rootTransform);
	float angle = glm::orientedAngle({ 0.0, 1.0f }, deltaForward);
	
	glm::vec2 deltaPos = MathUtils::PositionTo(nextRootTransform, rootTransform);

	glm::vec3 delta = { deltaPos, angle };
	Y_SequenceDeltaUpdate.push_back(delta);
}

std::vector<glm::quat> ModeAdaptivePreprocessPlugin::prepareRootRotation(std::shared_ptr<PoseSequence> poseSequenceIn, std::shared_ptr<Skeleton> skeleton)
{

	// Get Bone Index for Hip and Shoulder. Currently hardcoded, should be changed to be more flexible.
	int rightHipIdx = skeleton->bone_names.at("pelvis_R");
	int leftHipIdx = skeleton->bone_names.at("pelvis_L");

	int rightShoulderIdx = skeleton->bone_names.at("shoulder_R");
	int leftShoulderIdx = skeleton->bone_names.at("shoulder_L");

	/*qDebug() << "Right Hip Index: " << rightHipIdx;
	qDebug() << "Left Hip Index: " << leftHipIdx;
	qDebug() << "Right Shoulder Index: " << rightShoulderIdx;
	qDebug() << "Left Shoulder Index: " << leftShoulderIdx;*/

	 std::function<glm::quat(int)> calculateRootRotation = [&](int frame) {

		glm::vec3 hipRight, hipLeft, shoulderRight, shoulderLeft;

		hipRight = poseSequenceIn->GetPositionAtFrame3D(frame, rightHipIdx);
		hipLeft = poseSequenceIn->GetPositionAtFrame3D(frame, leftHipIdx);


		glm::vec3 hipVector = hipRight - hipLeft;
		hipVector = glm::normalize(AnimHostHelper::ProjectPointOnGroundPlane(hipVector));

		shoulderRight = poseSequenceIn->GetPositionAtFrame3D(frame, rightShoulderIdx);
		shoulderLeft = poseSequenceIn->GetPositionAtFrame3D(frame, leftShoulderIdx);

		glm::vec3 shoulderVector = shoulderRight - shoulderLeft;
		shoulderVector = glm::normalize(AnimHostHelper::ProjectPointOnGroundPlane(shoulderVector));

		glm::vec3 forward = glm::normalize(hipVector + shoulderVector);
		forward = glm::cross(glm::vec3(0.0, 1.0, 0.0), forward);
		forward = glm::normalize(AnimHostHelper::ProjectPointOnGroundPlane(forward));

		glm::quat rotation = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), forward);

		return rotation;
	};

    // Calculate Root Rotation for each frame
	std::vector<glm::quat> rootRotations;
	rootRotations.reserve(poseSequenceIn->mPoseSequence.size());

	for (int i = 0; i < poseSequenceIn->mPoseSequence.size(); i++) {
		rootRotations.push_back(calculateRootRotation(i));
	}

	return rootRotations;
}

std::vector<glm::mat4> ModeAdaptivePreprocessPlugin::prepareBipedRoot(std::shared_ptr<PoseSequence> poseSequenceIn, std::shared_ptr<Skeleton> skeleton)
{

	int numFrames = poseSequenceIn->mPoseSequence.size();

	int hipIdx = skeleton->bone_names.at("hip"); 

	std::vector<glm::quat> rootRot = prepareRootRotation(poseSequenceIn, skeleton);

	std::vector<glm::vec3> rootPos = std::vector<glm::vec3>(numFrames);

	for (int i = 0; i < numFrames; i++) {
		rootPos[i] = poseSequenceIn->mPoseSequence[i].mPositionData[hipIdx];
	}
	
	std::vector<glm::mat4> rootTransforms = std::vector<glm::mat4>(numFrames);

	for (int i = 0; i < numFrames; i++) {
		glm::vec3 pos = glm::vec3(rootPos[i].x, 0.f,rootPos[i].z);
		glm::quat rot = rootRot[i];

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * glm::toMat4(rot);

		rootTransforms[i] = transform;
	}


	return rootTransforms;
}



std::vector<float> ModeAdaptivePreprocessPlugin::prepareTrajectoryData(int referenceFrame, std::shared_ptr<Animation> animation, glm::mat4 Root,bool isOutput)
{

	std::vector<glm::vec2> posTrajectory = std::vector<glm::vec2>(numSamples);
	std::vector<glm::vec2> forwardTrajectory = std::vector<glm::vec2>(numSamples);
	std::vector<glm::vec2> velTrajectory = std::vector<glm::vec2>(numSamples);
	std::vector<float> desSpeedTrajectory = std::vector<float>(numSamples, 0.0f);

	
	int refIdx = referenceFrame + (isOutput ? 1 : 0);

	// For output data, we only need to calculate the trajectory for the future steps.
	// 6 is the start index for future steps, including pivot of frame range. Might need to change this if the number of samples changes.
	int startIdx = isOutput ? 7 : 0;

	FrameRange frameRange(numSamples, 60, refIdx, startIdx);

	// counter to keep track of the current sample index regardless of the frame range.
	// requered for indexing data and flattening the data.
	int sampleCounter = 0;

	for (int frameIdx : frameRange) {

		//qDebug() << "Frame Index: " << frameIdx;

		//NEW TRAJECTORY DATA
		//glm::mat4 frameRoot = animation->CalculateRootTransform(frameIdx, rootbone_idx);
		glm::mat4 frameRoot = rootBoneTransforms[glm::min(int(rootBoneTransforms.size()-1),frameIdx)];

		//relative Pos
		glm::vec2 relativePos = MathUtils::PositionTo(frameRoot, Root);
		posTrajectory[sampleCounter] = relativePos;

		//relative Forward
		glm::vec2 relativeForward = MathUtils::ForwardTo(frameRoot, Root);
		forwardTrajectory[sampleCounter] =	relativeForward;

		//relative Velocity
		glm::vec3 cPos = frameRoot * glm::vec4(0.0, 0.0, 0.0, 1.0);
		//glm::mat4 pFrameRoot = animation->CalculateRootTransform(glm::max(0, frameIdx - 1), rootbone_idx);
		glm::mat4 pFrameRoot = rootBoneTransforms[glm::max(0, glm::min(int(rootBoneTransforms.size() - 1), frameIdx-1))];
		glm::vec3 pPos = pFrameRoot * glm::vec4(0.0, 0.0, 0.0, 1.0);
		glm::vec3 v = (cPos - pPos) / (1.f / 60.f); // Velocity Unit: cm/s)

		glm::vec3 relativeVelocity = MathUtils::VelocityTo(v, Root);
		velTrajectory[sampleCounter] = glm::vec2( relativeVelocity.x, relativeVelocity.z ) / 100.f; // Velocity Unit: m/s

		// Speed
		desSpeedTrajectory[sampleCounter] = glm::length(velTrajectory[sampleCounter]);

		sampleCounter++;
	}

	// Flatten Root Trajectory data into single vector.

	std::vector<float> flatTrajectoryData;
	for (int i = 0; i < sampleCounter; i++) {
		flatTrajectoryData.push_back(posTrajectory[i].x);
		flatTrajectoryData.push_back(posTrajectory[i].y);
		flatTrajectoryData.push_back(forwardTrajectory[i].x);
		flatTrajectoryData.push_back(forwardTrajectory[i].y);
		flatTrajectoryData.push_back(velTrajectory[i].x);
		flatTrajectoryData.push_back(velTrajectory[i].y);
		flatTrajectoryData.push_back(desSpeedTrajectory[i]);
	}



	return flatTrajectoryData;
}

std::vector<glm::vec3> ModeAdaptivePreprocessPlugin::prepareJointPositions(int referenceFrame, std::shared_ptr<PoseSequence> poseSequenceIn, glm::mat4 Root, bool isOutput)
{

	//if output we need to offset the frame index by 1
	int frameIdx = referenceFrame + (isOutput ? 1 : 0);

	std::vector<glm::vec3> relativeJointPosition = std::vector<glm::vec3>(poseSequenceIn->mPoseSequence[frameIdx].mPositionData.size());

	for (int i = 0; i < poseSequenceIn->mPoseSequence[frameIdx].mPositionData.size(); i++) {
		glm::vec3 samplePosition = poseSequenceIn->mPoseSequence[frameIdx].mPositionData[i];

		relativeJointPosition[i] = MathUtils::PositionTo(samplePosition, Root);
	}

	return relativeJointPosition;
}

std::vector<glm::quat> ModeAdaptivePreprocessPlugin::prepareJointRotations(int referenceFrame, std::shared_ptr<Animation> animation, std::shared_ptr<Skeleton> skeleton, glm::mat4 Root, bool isOutput)
{
	std::vector<glm::quat> relativeJointRotations = std::vector<glm::quat>(skeleton->mNumBones);
	std::vector<glm::mat4> transforms;

	int frameIdx = referenceFrame + (isOutput ? 1 : 0);

	AnimHostHelper::ForwardKinematics(*skeleton, *animation, transforms, referenceFrame);

	for (int i = 0; i < transforms.size(); i++) {

		glm::mat4 relativeTransform = MathUtils::RelativeTransform(transforms[i], Root);

		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;

		glm::decompose(relativeTransform, scale, rotation, translation, skew, perspective);

		relativeJointRotations[i] = rotation;
	}

	return relativeJointRotations;
}

std::vector<Rotation6D> ModeAdaptivePreprocessPlugin::prepareJointRotations6D(int referenceFrame, std::shared_ptr<Animation> animation, std::shared_ptr<Skeleton> skeleton, glm::mat4 Root, bool isOutput)
{
	std::vector<Rotation6D> relativeJointRotations = std::vector<Rotation6D>(skeleton->mNumBones);
	std::vector<glm::mat4> transforms;

	int frameIdx = referenceFrame + (isOutput ? 1 : 0);

	AnimHostHelper::ForwardKinematics(*skeleton, *animation, transforms, referenceFrame);

	for (int i = 0; i < transforms.size(); i++) {

		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;

		glm::decompose(transforms[i], scale, rotation, translation, skew, perspective);

		glm::mat4 rotMat = glm::toMat4(rotation);

		glm::vec3 coll1 = glm::normalize(glm::vec3(rotMat[0].x, rotMat[0].y, rotMat[0].z));
		glm::vec3 coll2 = glm::normalize(glm::vec3(rotMat[1].x, rotMat[1].y, rotMat[1].z));

		coll1 = MathUtils::DirectionTo(coll1, Root);
		coll2 = MathUtils::DirectionTo(coll2, Root);

		Rotation6D rot6D = { coll1.x, coll1.y, coll1.z, coll2.x, coll2.y, coll2.z };

		relativeJointRotations[i] = rot6D;
	}

	return relativeJointRotations;
}

std::vector<glm::vec3> ModeAdaptivePreprocessPlugin::prepareJointVelocities(int referenceFrame, std::shared_ptr<JointVelocitySequence> velSeq, glm::mat4 Root, bool isOutput)
{
	int frameIdx = referenceFrame + (isOutput ? 1 : 0);
	// Joint Velocities
	std::vector<glm::vec3> relativeJointVelocities = std::vector<glm::vec3>(velSeq->mJointVelocitySequence[frameIdx].mJointVelocity.size());

	for (int i = 0; i < relativeJointVelocities.size(); i++) {
		relativeJointVelocities[i] = MathUtils::VelocityTo(velSeq->mJointVelocitySequence[frameIdx].mJointVelocity[i], Root);
	}

	return relativeJointVelocities;

}

std::shared_ptr<NodeData> ModeAdaptivePreprocessPlugin::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

QWidget* ModeAdaptivePreprocessPlugin::embeddedWidget()
{
	if (!_widget) {
		_widget = new QWidget();
		_boneSelect = new BoneSelectionWidget(_widget);
		_folderSelect = new FolderSelectionWidget(_widget);
		_cbOverwrite = new QCheckBox("Overwrite Existing Data");


		QVBoxLayout* layout = new QVBoxLayout();


		layout->addWidget(_folderSelect);
		layout->addWidget(_cbOverwrite);
		layout->addWidget(_boneSelect);

		_widget->setLayout(layout);
		//_widget->setMinimumHeight(_widget->sizeHint().height());
		//_widget->setMaximumWidth(_widget->sizeHint().width());

		connect(_boneSelect, &BoneSelectionWidget::currentBoneChanged, this, &ModeAdaptivePreprocessPlugin::onRootBoneSelectionChanged);
		connect(_folderSelect, &FolderSelectionWidget::directoryChanged, this, &ModeAdaptivePreprocessPlugin::onFolderSelectionChanged);
		connect(_cbOverwrite, &QCheckBox::stateChanged, this, &ModeAdaptivePreprocessPlugin::onOverrideCheckbox);

		_widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
			"QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
			"QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
			"QLabel{padding: 5px;}"
			"QComboBox{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
			"QComboBox::drop-down{"
			"background-color:rgb(98, 139, 202);"
			"subcontrol-origin: padding;"
			"subcontrol-position: top right;"
			"width: 15px;"
			"border-top-right-radius: 4px;"
			"border-bottom-right-radius: 4px;}"
			"QComboBox QAbstractItemView{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-bottom-right-radius: 4px; border-bottom-left-radius: 4px; padding: 0px;}"
			"QScrollBar:vertical {"
			"border: 1px rgb(25, 25, 25);"
			"background:rgb(25, 25, 25);"
			"border-radius: 2px;"
			"width:6px;"
			"margin: 2px 0px 2px 1px;}"
			"QScrollBar::handle:vertical {"
			"border-radius: 2px;"
			"min-height: 0px;"
			"background-color: rgb(25, 25, 25);}"
			"QScrollBar::add-line:vertical {"
			"height: 0px;"
			"subcontrol-position: bottom;"
			"subcontrol-origin: margin;}"
			"QScrollBar::sub-line:vertical {"
			"height: 0px;"
			"subcontrol-position: top;"
			"subcontrol-origin: margin;}"
		);
	}

	return _widget;
}

void ModeAdaptivePreprocessPlugin::clearExistingData()
{
	if (bOverwriteDataExport) {
		// Delete Existing Files
		FileHandler<QDataStream>::deleteFile(exportDirectory + metadataFileName);
		FileHandler<QDataStream>::deleteFile(exportDirectory + sequencesFileName);
		FileHandler<QDataStream>::deleteFile(exportDirectory + dataXFileName);
		FileHandler<QDataStream>::deleteFile(exportDirectory + dataYFileName);

		bOverwriteDataExport = false;
		_cbOverwrite->setCheckState(Qt::Unchecked);
	}
}

void ModeAdaptivePreprocessPlugin::writeMetaData() {

	if (auto sp_poseSeq = _poseSequenceIn.lock()) {
		auto poseSequenceIn = sp_poseSeq->getData();

		if (auto sp_animation = _animationIn.lock()) {
			auto animation = sp_animation->getData();

			if (auto sp_velSeq = _jointVelocitySequenceIn.lock()) {
				auto velSeq = sp_velSeq->getData();

				if (auto sp_skeleton = _skeletonIn.lock()) {
					auto skeleton = sp_skeleton->getData();

					// DO ONCE
					QString filenameMetadata = exportDirectory + metadataFileName;
					QFile metaFile(filenameMetadata);

					metaFile.open(QIODevice::WriteOnly | QIODevice::Text);
					QTextStream out(&metaFile);
					QString header = "";

					//TODO: Write total amount frames

					//Write Header Input Features
					
					int featureCount = 0;


					for (int i = 0; i < numSamples; i++) {
						header += ",root_pos_x_" + QString::number(i);
						header += ",root_pos_y_" + QString::number(i);
						header += ",root_fwd_x_" + QString::number(i);
						header += ",root_fwd_y_" + QString::number(i);
						header += ",root_vel_x_" + QString::number(i);
						header += ",root_vel_y_" + QString::number(i);
						header += ",root_speed_" + QString::number(i);

						featureCount += 7;
					};

					qDebug() << poseSequenceIn->mPoseSequence[0].mPositionData.size() << "  and bone Size:" << skeleton->mNumBones;
					for (int i = 0; i < poseSequenceIn->mPoseSequence[0].mPositionData.size(); i++) {
						QString boneName = QString::fromStdString(skeleton->bone_names_reverse.at(i));
						header += ",jpos_x_" + boneName;
						header += ",jpos_y_" + boneName;
						header += ",jpos_z_" + boneName;

						/*header += ",jrot_x_" + boneName;
						header += ",jrot_y_" + boneName;
						header += ",jrot_z_" + boneName;
						header += ",jrot_W_" + boneName;*/
						header += ",jrot_0_" + boneName;
						header += ",jrot_1_" + boneName;
						header += ",jrot_2_" + boneName;
						header += ",jrot_3_" + boneName;
						header += ",jrot_4_" + boneName;
						header += ",jrot_5_" + boneName;

						header += ",jvel_x_" + boneName;
						header += ",jvel_y_" + boneName;
						header += ",jvel_z_" + boneName;

						featureCount += 12;

					}
					header += "\n";
					
					out << QString::number(featureCount);
					out << header;

					//Write Header Output Features
					header = "";
					featureCount = 0;

					header += ",delta_x";
					header += ",delta_y";
					header += ",delta_angle";

					featureCount += 3;

					//Root Trajectory. start index at 6 for future steps
					for (int i = 7; i < numSamples; i++) {
						header += ",out_root_pos_x_" + QString::number(i);
						header += ",out_root_pos_y_" + QString::number(i);
						header += ",out_root_fwd_x_" + QString::number(i);
						header += ",out_root_fwd_y_" + QString::number(i);
						header += ",out_root_vel_x_" + QString::number(i);
						header += ",out_root_vel_y_" + QString::number(i);
						header += ",out_root_speed_" + QString::number(i);

						featureCount += 7;
					};

					qDebug() << poseSequenceIn->mPoseSequence[0].mPositionData.size() << " and bone size:" << skeleton->mNumBones;
					for (int i = 0; i < poseSequenceIn->mPoseSequence[0].mPositionData.size(); i++) {
						QString boneName = QString::fromStdString(skeleton->bone_names_reverse.at(i));
						header += ",out_jpos_x_" + boneName;
						header += ",out_jpos_y_" + boneName;
						header += ",out_jpos_z_" + boneName;

						/*header += ",out_jrot_x_" + boneName;
						header += ",out_jrot_y_" + boneName;
						header += ",out_jrot_z_" + boneName;
						header += ",out_jrot_W_" + boneName;*/
						header += ",out_jrot_0_" + boneName;
						header += ",out_jrot_1_" + boneName;
						header += ",out_jrot_2_" + boneName;
						header += ",out_jrot_3_" + boneName;
						header += ",out_jrot_4_" + boneName;
						header += ",out_jrot_5_" + boneName;

						header += ",out_jvel_x_" + boneName;
						header += ",out_jvel_y_" + boneName;
						header += ",out_jvel_z_" + boneName;

						featureCount += 12;

					}
					header += "\n";

					out << QString::number(featureCount);
					out << header;

					metaFile.close();

				}
			}
		}
	}
};

void ModeAdaptivePreprocessPlugin::writeInputData()
{
	if (auto sp_poseSeq = _poseSequenceIn.lock()) {
		auto poseSequenceIn = sp_poseSeq->getData();

		if (auto sp_animation = _animationIn.lock()) {
			auto animation = sp_animation->getData();

			if (auto sp_velSeq = _jointVelocitySequenceIn.lock()) {
				auto velSeq = sp_velSeq->getData();

				if (auto sp_skeleton = _skeletonIn.lock()) {
					auto skeleton = sp_skeleton->getData();

					qDebug() << "Write Pose Data to Binary File";

					//Write Identifier to txt
					QString fileNameIdent = exportDirectory + sequencesFileName;

					FileHandler<QTextStream> fileIdent = FileHandler<QTextStream>(fileNameIdent, true);
					QTextStream& outID = fileIdent.getStream();

					QElapsedTimer timer;
					timer.start();

					QString idString = "";

					for (int idx = 0; idx < rootSequenceData.size(); idx++) {
						idString += QString::number(poseSequenceIn->sequenceID) + " ";
						idString += QString::number(pastSamples + idx) + " ";
						idString += "Standard ";
						idString += poseSequenceIn->sourceName + " ";
						idString += poseSequenceIn->dataSetID;
						idString += "\n";

						outID << idString;

						idString = "";
					}

					// Write data to binary
					QString fileNameData = exportDirectory + dataXFileName;
					FileHandler<QDataStream> fileData = FileHandler<QDataStream>(fileNameData);

					QDataStream& outData = fileData.getStream();
					outData.setByteOrder(QDataStream::LittleEndian);
					int countFeatures = (rootSequenceData[0].size() + (poseSequenceIn->mPoseSequence[0].mPositionData.size() * 12));
					int sizeFrame = countFeatures * sizeof(float);
					std::vector<float> flattenedOutput(countFeatures, 0.0f);

					for (int idx = 0; idx < rootSequenceData.size(); idx++) {
						int fltIdx = 0;
						for (int i = 0; i < rootSequenceData[idx].size(); i++) {
							flattenedOutput[fltIdx + i] = rootSequenceData[idx][i];
						}

						fltIdx += rootSequenceData[idx].size();

						for (int i = 0; i < poseSequenceIn->mPoseSequence[0].mPositionData.size(); i++) {

							flattenedOutput[fltIdx] = sequenceRelativeJointPosition[idx][i].x;
							flattenedOutput[fltIdx + 1] = sequenceRelativeJointPosition[idx][i].y;
							flattenedOutput[fltIdx + 2] = sequenceRelativeJointPosition[idx][i].z;

							flattenedOutput[fltIdx + 3] = sequenceRelativJointRotations6D[idx][i][0];
							flattenedOutput[fltIdx + 4] = sequenceRelativJointRotations6D[idx][i][1];
							flattenedOutput[fltIdx + 5] = sequenceRelativJointRotations6D[idx][i][2];
							flattenedOutput[fltIdx + 6] = sequenceRelativJointRotations6D[idx][i][3];
							flattenedOutput[fltIdx + 7] = sequenceRelativJointRotations6D[idx][i][4];
							flattenedOutput[fltIdx + 8] = sequenceRelativJointRotations6D[idx][i][5];

							flattenedOutput[fltIdx + 9] = sequenceRelativeJointVelocities[idx][i].x;
							flattenedOutput[fltIdx + 10] = sequenceRelativeJointVelocities[idx][i].y;
							flattenedOutput[fltIdx + 11] = sequenceRelativeJointVelocities[idx][i].z;

							fltIdx += 12;
						}

						int byteswritten = outData.writeRawData(reinterpret_cast<const char*>(flattenedOutput.data()), sizeFrame);
					}

					auto elapsed = timer.elapsed();
					qDebug() << "Time taken by the code snippet: " << elapsed << "milliseconds";
				}
			}
		}
	}
}

void ModeAdaptivePreprocessPlugin::writeOutputData()
{
	if (auto sp_poseSeq = _poseSequenceIn.lock()) {
		auto poseSequenceIn = sp_poseSeq->getData();

		if (auto sp_animation = _animationIn.lock()) {
			auto animation = sp_animation->getData();

			if (auto sp_velSeq = _jointVelocitySequenceIn.lock()) {
				auto velSeq = sp_velSeq->getData();

				if (auto sp_skeleton = _skeletonIn.lock()) {
					auto skeleton = sp_skeleton->getData();


					QString fileNameData = exportDirectory + dataYFileName;
					FileHandler<QDataStream> fileData = FileHandler<QDataStream>(fileNameData);

					QDataStream& outData = fileData.getStream();
					int countFeatures = Y_RootSequenceData.size() * ( 3 + Y_RootSequenceData[0].size() + (poseSequenceIn->mPoseSequence[0].mPositionData.size() * 12));
					int sizeFrame = countFeatures * sizeof(float);
					std::vector<float> flattenedOutput(countFeatures); 

					int fltIdx = 0;

					for (int idx = 0; idx < Y_RootSequenceData.size(); idx++) {

						flattenedOutput[fltIdx] = Y_SequenceDeltaUpdate[idx].x;
						flattenedOutput[fltIdx+1] = Y_SequenceDeltaUpdate[idx].y;
						flattenedOutput[fltIdx+2] = Y_SequenceDeltaUpdate[idx].z;
						fltIdx += 3;

						for (int i = 0; i < Y_RootSequenceData[idx].size(); i++) {
							flattenedOutput[fltIdx+i] = Y_RootSequenceData[idx][i];
						}

						fltIdx += Y_RootSequenceData[idx].size();

						for (int i = 0; i < poseSequenceIn->mPoseSequence[0].mPositionData.size(); i++) {
							flattenedOutput[fltIdx] += Y_SequenceRelativeJointPosition[idx][i].x;
							flattenedOutput[fltIdx + 1] = Y_SequenceRelativeJointPosition[idx][i].y;
							flattenedOutput[fltIdx + 2] = Y_SequenceRelativeJointPosition[idx][i].z;

							flattenedOutput[fltIdx + 3] = Y_SequenceRelativJointRotations6D[idx][i][0];
							flattenedOutput[fltIdx + 4] = Y_SequenceRelativJointRotations6D[idx][i][1];
							flattenedOutput[fltIdx + 5] = Y_SequenceRelativJointRotations6D[idx][i][2];
							flattenedOutput[fltIdx + 6] = Y_SequenceRelativJointRotations6D[idx][i][3];
							flattenedOutput[fltIdx + 7] = Y_SequenceRelativJointRotations6D[idx][i][4];
							flattenedOutput[fltIdx + 8] = Y_SequenceRelativJointRotations6D[idx][i][5];

							flattenedOutput[fltIdx + 9] = Y_SequenceRelativeJointVelocities[idx][i].x;
							flattenedOutput[fltIdx + 10] = Y_SequenceRelativeJointVelocities[idx][i].y;
							flattenedOutput[fltIdx + 11] = Y_SequenceRelativeJointVelocities[idx][i].z;
							
							fltIdx += 12;
						}			
					}

					outData.writeRawData((char*)&flattenedOutput[0], sizeFrame);
					
				}
			}
		}
	}
}

void ModeAdaptivePreprocessPlugin::onFolderSelectionChanged()
{
	
	exportDirectory = _folderSelect->GetSelectedDirectory() + "/";

	Q_EMIT embeddedWidgetSizeUpdated();
}

void ModeAdaptivePreprocessPlugin::onRootBoneSelectionChanged(const int indx)
{
	if (auto sp_skeleton = _skeletonIn.lock()) {
		auto skeleton = sp_skeleton->getData();

		auto selected = skeleton->bone_names.at(_boneSelect->GetSelectedBone().toStdString());

		if (rootbone_idx != selected) {
			rootbone_idx = selected;

			qDebug() << "selected bone list idx" << selected << " selcetion " << _boneSelect->GetSelectedBone();

			run();
		}		
	}
}

void ModeAdaptivePreprocessPlugin::onOverrideCheckbox(int state) 
{
	bOverwriteDataExport = state;
}


