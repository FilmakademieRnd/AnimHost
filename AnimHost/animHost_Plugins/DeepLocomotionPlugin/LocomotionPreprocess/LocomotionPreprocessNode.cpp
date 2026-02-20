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
#include "LocomotionPreprocessNode.h"

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


LocomotionPreprocessNode::LocomotionPreprocessNode()
{
}

LocomotionPreprocessNode::~LocomotionPreprocessNode()
{
}

const SkeletonBoneConfig& LocomotionPreprocessNode::getBoneConfig() const
{
    return getSkeletonBoneConfig(_skeletonType);
}

unsigned int LocomotionPreprocessNode::nDataPorts(QtNodes::PortType portType) const
{
	if (portType == QtNodes::PortType::In)
		return 5;  // Skeleton, PoseSequence, JointVelocitySequence, Animation, ValidFrames
	else
		return 0;
}

NodeDataType LocomotionPreprocessNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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
		case 4:
			return AnimNodeData<ValidFrames>::staticType();
		default:
			break;
		}
	}

	return type;
}

QJsonObject LocomotionPreprocessNode::save() const
{
	QJsonObject nodeJson = NodeDelegateModel::save();

	nodeJson["dir"] = exportDirectory;
	nodeJson["skeletonType"] = static_cast<int>(_skeletonType);

	return nodeJson;
}

void LocomotionPreprocessNode::load(QJsonObject const& p)
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

	QJsonValue skeletonTypeVal = p["skeletonType"];
	if (!skeletonTypeVal.isUndefined()) {
		_skeletonType = static_cast<SkeletonType>(skeletonTypeVal.toInt());
		if (_skeletonTypeCombo) {
			_skeletonTypeCombo->setCurrentIndex(static_cast<int>(_skeletonType));
		}
	}
}


void LocomotionPreprocessNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
	qDebug() << "LocomotionPreprocessNode setInData";

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
			break;
		case 4:
			_validFramesIn.reset();
			break;

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
	case 4:
		_validFramesIn = std::static_pointer_cast<AnimNodeData<ValidFrames>>(data);
		break;
	default:
		return;
	}

}

bool LocomotionPreprocessNode::isDataAvailable() {
	return _poseSequenceIn.lock() && _animationIn.lock() && _jointVelocitySequenceIn.lock() && _skeletonIn.lock();
}

void LocomotionPreprocessNode::run()
{

	if (!isDataAvailable()) {
		qDebug() << "LocomotionPreprocessNode: Incomplete Input Data";
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


		qDebug() << "LocomotionPreprocessNode: Input Data Complete";

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

		qDebug() << "[LocomotionPreprocessNode] Input sizes:"
		         << "animation->mDurationFrames:" << animation->mDurationFrames
		         << "poseSequenceIn->mPoseSequence.size():" << poseSequenceIn->mPoseSequence.size()
		         << "velSeq->mJointVelocitySequence.size():" << velSeq->mJointVelocitySequence.size();

		// Get frames to process based on ValidFrames input
		std::vector<int> framesToProcess = getFramesToProcess(animation, animation->sourceName);

		if (framesToProcess.empty()) {
			qDebug() << "[LocomotionPreprocessNode] No frames to process";
			return;
		}

		// Segment frames into consecutive groups
		std::vector<std::vector<int>> segments = segmentConsecutiveFrames(framesToProcess);

		qDebug() << "[LocomotionPreprocessNode] Found" << segments.size()
		         << "consecutive segments from" << framesToProcess.size() << "total frames";

		// Reset index if overwriting
		if (bOverwriteDataExport) {
			currentSequenceIndex = 1;
		}

		// Process and write each segment separately
		for (size_t segIdx = 0; segIdx < segments.size(); segIdx++) {
			const std::vector<int>& currentSegment = segments[segIdx];

			qDebug() << "[LocomotionPreprocessNode] Processing segment" << (segIdx + 1)
			         << "with" << currentSegment.size() << "frames, SeqId:" << currentSequenceIndex;

			// Clear data for this segment
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
			processedFrameNumbers.clear();

			// Process all frames in this segment
			for (int frameCounter : currentSegment) {
				processFrame(frameCounter, poseSequenceIn, animation, velSeq, skeleton);
				processedFrameNumbers.push_back(frameCounter);
			}

			// Clear existing files only on first segment if overwriting
			if (segIdx == 0) {
				clearExistingData();
			}

			// Write this segment's data to files
			writeMetaData();
			writeInputData();
			writeOutputData();

			// Increment sequence index for next segment
			currentSequenceIndex++;
		}

	}
}

void LocomotionPreprocessNode::processFrame(int frameCounter, std::shared_ptr<PoseSequence> poseSequenceIn, std::shared_ptr<Animation> animation, std::shared_ptr<JointVelocitySequence> velSeq, std::shared_ptr<Skeleton> skeleton)
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

std::vector<glm::quat> LocomotionPreprocessNode::prepareRootRotation(std::shared_ptr<PoseSequence> poseSequenceIn, std::shared_ptr<Skeleton> skeleton)
{
	const auto& boneConfig = getBoneConfig();

	// Get Bone Index for rear and front joints based on skeleton type
	int rearRightIdx = skeleton->bone_names.at(boneConfig.rearRight);
	int rearLeftIdx = skeleton->bone_names.at(boneConfig.rearLeft);
	int frontRightIdx = skeleton->bone_names.at(boneConfig.frontRight);
	int frontLeftIdx = skeleton->bone_names.at(boneConfig.frontLeft);

	qDebug() << "Using skeleton type:" << (_skeletonType == SkeletonType::Bipedal ? "Bipedal" : "Quadrupedal");
	qDebug() << "Rear indices (R/L):" << rearRightIdx << "/" << rearLeftIdx;
	qDebug() << "Front indices (R/L):" << frontRightIdx << "/" << frontLeftIdx;

	std::function<glm::quat(int)> calculateRootRotation = [&](int frame) {

		glm::vec3 rearRight, rearLeft, frontRight, frontLeft;

		rearRight = poseSequenceIn->GetPositionAtFrame3D(frame, rearRightIdx);
		rearLeft = poseSequenceIn->GetPositionAtFrame3D(frame, rearLeftIdx);

		glm::vec3 rearVector = rearRight - rearLeft;
		rearVector = glm::normalize(AnimHostHelper::ProjectPointOnGroundPlane(rearVector));

		frontRight = poseSequenceIn->GetPositionAtFrame3D(frame, frontRightIdx);
		frontLeft = poseSequenceIn->GetPositionAtFrame3D(frame, frontLeftIdx);

		glm::vec3 frontVector = frontRight - frontLeft;
		frontVector = glm::normalize(AnimHostHelper::ProjectPointOnGroundPlane(frontVector));

		glm::vec3 forward = glm::normalize(rearVector + frontVector);
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

std::vector<glm::mat4> LocomotionPreprocessNode::prepareBipedRoot(std::shared_ptr<PoseSequence> poseSequenceIn, std::shared_ptr<Skeleton> skeleton)
{
	const auto& boneConfig = getBoneConfig();

	int numFrames = poseSequenceIn->mPoseSequence.size();

	int rootBoneIdx = skeleton->bone_names.at(boneConfig.rootBone);

	std::vector<glm::quat> rootRot = prepareRootRotation(poseSequenceIn, skeleton);

	std::vector<glm::quat> smoothedRootRot = rootRot;

	//smoothedRootRot = GaussianFilterQuaternions(smoothedRootRot, 30);

	std::vector<glm::vec3> rootPos = std::vector<glm::vec3>(numFrames);

	for (int i = 0; i < numFrames; i++) {
		rootPos[i] = poseSequenceIn->mPoseSequence[i].mPositionData[rootBoneIdx];
	}
	
	std::vector<glm::mat4> rootTransforms = std::vector<glm::mat4>(numFrames);

	for (int i = 0; i < numFrames; i++) {
		glm::vec3 pos = glm::vec3(rootPos[i].x, 0.f,rootPos[i].z);
		glm::quat rot = smoothedRootRot[i];

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * glm::toMat4(rot);

		rootTransforms[i] = transform;
	}


	return rootTransforms;
}



std::vector<float> LocomotionPreprocessNode::prepareTrajectoryData(int referenceFrame, std::shared_ptr<Animation> animation, glm::mat4 Root,bool isOutput)
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

std::vector<glm::vec3> LocomotionPreprocessNode::prepareJointPositions(int referenceFrame, std::shared_ptr<PoseSequence> poseSequenceIn, glm::mat4 Root, bool isOutput)
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

std::vector<glm::quat> LocomotionPreprocessNode::prepareJointRotations(int referenceFrame, std::shared_ptr<Animation> animation, std::shared_ptr<Skeleton> skeleton, glm::mat4 Root, bool isOutput)
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

std::vector<Rotation6D> LocomotionPreprocessNode::prepareJointRotations6D(int referenceFrame, std::shared_ptr<Animation> animation, std::shared_ptr<Skeleton> skeleton, glm::mat4 Root, bool isOutput)
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

std::vector<glm::vec3> LocomotionPreprocessNode::prepareJointVelocities(int referenceFrame, std::shared_ptr<JointVelocitySequence> velSeq, glm::mat4 Root, bool isOutput)
{
	int frameIdx = referenceFrame + (isOutput ? 1 : 0);
	// Joint Velocities
	std::vector<glm::vec3> relativeJointVelocities = std::vector<glm::vec3>(velSeq->mJointVelocitySequence[frameIdx].mJointVelocity.size());

	for (int i = 0; i < relativeJointVelocities.size(); i++) {
		relativeJointVelocities[i] = MathUtils::VelocityTo(velSeq->mJointVelocitySequence[frameIdx].mJointVelocity[i], Root);
	}

	return relativeJointVelocities;

}

std::shared_ptr<NodeData> LocomotionPreprocessNode::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

QWidget* LocomotionPreprocessNode::embeddedWidget()
{
	if (!_widget) {
		_widget = new QWidget();
		_boneSelect = new BoneSelectionWidget(_widget);
		_folderSelect = new FolderSelectionWidget(_widget);
		_cbOverwrite = new QCheckBox("Overwrite Existing Data");

		// Skeleton type selector
		_skeletonTypeCombo = new QComboBox(_widget);
		_skeletonTypeCombo->addItem("Bipedal", static_cast<int>(SkeletonType::Bipedal));
		_skeletonTypeCombo->addItem("Quadrupedal", static_cast<int>(SkeletonType::Quadrupedal));
		_skeletonTypeCombo->setCurrentIndex(static_cast<int>(_skeletonType));

		QVBoxLayout* layout = new QVBoxLayout();

		layout->addWidget(new QLabel("Skeleton Type:"));
		layout->addWidget(_skeletonTypeCombo);
		layout->addWidget(_folderSelect);
		layout->addWidget(_cbOverwrite);
		layout->addWidget(_boneSelect);

		_widget->setLayout(layout);

		connect(_skeletonTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LocomotionPreprocessNode::onSkeletonTypeChanged);
		connect(_boneSelect, &BoneSelectionWidget::currentBoneChanged, this, &LocomotionPreprocessNode::onRootBoneSelectionChanged);
		connect(_folderSelect, &FolderSelectionWidget::directoryChanged, this, &LocomotionPreprocessNode::onFolderSelectionChanged);
		connect(_cbOverwrite, &QCheckBox::stateChanged, this, &LocomotionPreprocessNode::onOverrideCheckbox);

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

void LocomotionPreprocessNode::clearExistingData()
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

void LocomotionPreprocessNode::writeMetaData() {

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

void LocomotionPreprocessNode::writeInputData()
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
						// Use currentSequenceIndex and actual frame numbers from processedFrameNumbers
						idString += QString::number(currentSequenceIndex) + " ";
						idString += QString::number(processedFrameNumbers[idx]) + " ";
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

void LocomotionPreprocessNode::writeOutputData()
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

void LocomotionPreprocessNode::onFolderSelectionChanged()
{
	
	exportDirectory = _folderSelect->GetSelectedDirectory() + "/";

	Q_EMIT embeddedWidgetSizeUpdated();
}

void LocomotionPreprocessNode::onRootBoneSelectionChanged(const int indx)
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

void LocomotionPreprocessNode::onOverrideCheckbox(int state)
{
	bOverwriteDataExport = state;
}

void LocomotionPreprocessNode::onSkeletonTypeChanged(int index)
{
	SkeletonType newType = static_cast<SkeletonType>(index);
	if (_skeletonType != newType) {
		_skeletonType = newType;
		const auto& config = getBoneConfig();
		qDebug() << "Skeleton type changed to:" << (index == 0 ? "Bipedal" : "Quadrupedal");
		qDebug() << "  Root bone:" << config.rootBone;
		qDebug() << "  Rear bones:" << config.rearLeft << "/" << config.rearRight;
		qDebug() << "  Front bones:" << config.frontLeft << "/" << config.frontRight;
	}
}

// Experimental

// Helper function to apply Gaussian filter
std::vector<glm::quat> LocomotionPreprocessNode::ApplyGaussianFilter(const std::vector<glm::quat>& rotations, const std::vector<float>& weights, float sigma) {
	std::vector<glm::quat> smoothedRotations(rotations.size());

	int kernelRadius = static_cast<int>(3 * sigma); // Kernel size based on standard deviation
	int frameCount = rotations.size();

	for (int i = 0; i < frameCount; i++) {
		glm::quat weightedSum(0, 0, 0, 0);
		float totalWeight = 0.0f;

		for (int j = -kernelRadius; j <= kernelRadius; j++) {
			int idx = glm::clamp(i + j, 0, frameCount - 1);
			float weight = std::exp(-(j * j) / (2 * sigma * sigma)) * weights[idx];

			weightedSum += weight * rotations[idx];
			totalWeight += weight;
		}

		smoothedRotations[i] = glm::normalize(weightedSum / totalWeight);
	}

	return smoothedRotations;
}

// Function to compute adaptive weights
std::vector<float> LocomotionPreprocessNode::ComputeAdaptiveWeights(const std::vector<glm::quat>& rotations, float sigma) {
	std::vector<float> weights(rotations.size());
	int frameCount = rotations.size();

	for (int i = 1; i < frameCount; i++) {
		glm::quat delta = glm::inverse(rotations[i - 1]) * rotations[i];
		float angle = 2.0f * std::acos(glm::clamp(delta.w, -1.0f, 1.0f));

		// Use the magnitude of the derivative as the weight
		weights[i] = std::exp(-angle * angle / (2 * sigma * sigma));
	}

	weights[0] = weights[1]; // Initialize the first weight
	return weights;
}

// Main function to adaptively smooth root rotations
std::vector<glm::quat> LocomotionPreprocessNode::SmoothRootRotations(const std::vector<glm::quat>& rootRotations, float timeWindow) {
	// Compute the standard deviation for the Gaussian filter
	float sigma = timeWindow / 4.0f;

	// Step 1: Compute the adaptive weights
	std::vector<float> weights = ComputeAdaptiveWeights(rootRotations, sigma);

	// Step 2: Apply the Gaussian filter using the adaptive weights
	std::vector<glm::quat> smoothedRotations = ApplyGaussianFilter(rootRotations, weights, sigma);

	return smoothedRotations;
}


// Gaussian filter for quaternions
std::vector<glm::quat> LocomotionPreprocessNode::GaussianFilterQuaternions(const std::vector<glm::quat>& quaternions, float sigma) {
	std::vector<glm::quat> smoothedQuaternions(quaternions.size());

	// Calculate the kernel radius based on the standard deviation
	int kernelRadius = static_cast<int>(std::ceil(3.0f * sigma));

	for (int i = 0; i < quaternions.size(); ++i) {
		glm::quat weightedSum(0.0f, 0.0f, 0.0f, 0.0f);
		float totalWeight = 0.0f;

		// Iterate over the kernel window centered around the current sample
		for (int j = -kernelRadius; j <= kernelRadius; ++j) {
			int idx = std::clamp(i + j, 0, static_cast<int>(quaternions.size()) - 1);
			float distance = static_cast<float>(j);

			// Calculate Gaussian weight
			float weight = std::exp(-(distance * distance) / (2.0f * sigma * sigma));

			// Accumulate the weighted quaternion using SLERP for interpolation
			weightedSum = glm::slerp(weightedSum, quaternions[idx], weight);
			totalWeight += weight;
		}

		// Normalize the result and store it
		smoothedQuaternions[i] = glm::normalize(weightedSum);
	}

	return smoothedQuaternions;
}

std::vector<int> LocomotionPreprocessNode::getFramesToProcess(std::shared_ptr<Animation> animation, const QString& sourceName)
{
	std::vector<int> frames;
	int duration = animation->mDurationFrames;

	// Check if ValidFrames is available
	auto sp_validFrames = _validFramesIn.lock();

	if (!sp_validFrames || sp_validFrames->getData()->isEmpty()) {
		// No Sequences.txt configured - use current behavior (all frames with 60-frame offset)
		for (int f = 60; f <= duration - 60; f++) {
			frames.push_back(f);
		}

		qDebug() << "[LocomotionPreprocessNode] No ValidFrames - processing"
		         << frames.size() << "frames (60-frame offset)";

		return frames;
	}

	// ValidFrames is configured - iterate only on valid frames
	auto validFrames = sp_validFrames->getData();
	QString stem = extractFileStem(sourceName);

	if (!validFrames->hasFile(stem)) {
		// File not in Sequences.txt - skip it
		qWarning() << "[LocomotionPreprocessNode] File" << stem
		           << "not found in Sequences.txt - skipping (no valid frames)";
		return frames;  // Empty
	}

	// Get valid frames for this file
	std::vector<int> validFrameList = validFrames->getFrames(stem);

	qDebug() << "[LocomotionPreprocessNode] Found" << validFrameList.size()
	         << "valid frames for" << stem << "in Sequences.txt";

	// Filter by scene bounds (need 60 frames for trajectory context)
	for (int f : validFrameList) {
		if (f >= 60 && f <= duration - 60) {
			frames.push_back(f);
		}
	}

	qDebug() << "[LocomotionPreprocessNode] After scene bounds check:"
	         << frames.size() << "frames to process";

	return frames;
}

std::vector<std::vector<int>> LocomotionPreprocessNode::segmentConsecutiveFrames(const std::vector<int>& frames) const
{
	std::vector<std::vector<int>> segments;
	if (frames.empty()) return segments;

	std::vector<int> currentSegment = {frames[0]};

	for (size_t i = 1; i < frames.size(); i++) {
		if (frames[i] == frames[i-1] + 1) {
			currentSegment.push_back(frames[i]);  // Consecutive
		} else {
			segments.push_back(currentSegment);   // Gap detected - save and start new
			currentSegment = {frames[i]};
		}
	}
	segments.push_back(currentSegment);  // Add last segment
	return segments;
}

QString LocomotionPreprocessNode::extractFileStem(const QString& sourceName) const
{
	// Remove path if present
	QString filename = QFileInfo(sourceName).fileName();

	// Remove extension
	int dotIndex = filename.lastIndexOf('.');
	if (dotIndex > 0) {
		return filename.left(dotIndex);
	}
	return filename;
}

