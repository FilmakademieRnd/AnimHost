
#define GLM_FORCE_SWIZZLE
#include "ModeAdaptivePreprocessPlugin.h"

#include <QElapsedTimer>
#include <FileHandler.h>


#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <commondatatypes.h>
#include <animhosthelper.h>


ModeAdaptivePreprocessPlugin::ModeAdaptivePreprocessPlugin()
{
	qDebug() << "ModeAdaptivePreprocessPlugin created";
}

ModeAdaptivePreprocessPlugin::~ModeAdaptivePreprocessPlugin()
{
	qDebug() << "~ModeAdaptivePreprocessPlugin()";
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

void ModeAdaptivePreprocessPlugin::run()
{

	if (auto sp_poseSeq = _poseSequenceIn.lock()) {
		auto poseSequenceIn = sp_poseSeq->getData();

		if (auto sp_animation = _animationIn.lock()) {
			auto animation = sp_animation->getData();

			if (auto sp_velSeq = _jointVelocitySequenceIn.lock()) {
				auto velSeq = sp_velSeq->getData();

				if (auto sp_skeleton = _skeletonIn.lock()) {
					auto skeleton = sp_skeleton->getData();

					
					
					rootSequenceData.clear();
					sequenceRelativeJointPosition.clear();
					sequenceRelativeJointVelocities.clear();
					sequenceRelativJointRotations.clear();

					Y_RootSequenceData.clear();
					Y_SequenceRelativeJointPosition.clear();
					Y_SequenceRelativeJointVelocities.clear();
					Y_SequenceRelativJointRotations.clear();


					
					int referenceFrame = 0; //Sampled Trajectory is centered around  referencFrame
					int pastFrameStartIdx = 0;

					//Offset to start of sequence, allows for enough frames to be left for past trajectory samples
					int start = pastSamples;

					//Offset to last frame of sequence, allows for enough frames to be left for trajectory 
					//and output (trajectory of next frame)
					int end = animation->mDurationFrames - futureSamples - 2;

					for (int frameCounter = start; frameCounter <= end; frameCounter++) {

						referenceFrame = frameCounter;
						pastFrameStartIdx = referenceFrame - pastSamples;

						fprintf(stderr, QString("\r Processing Frame: %1 / %2").arg(frameCounter).arg(end).toLatin1().data());

						//Prepare Root Positions. 2D Trajectory positions ground plane.
						std::vector<glm::vec2> posTrajectory = std::vector<glm::vec2>(numSamples);
						glm::vec2 curretRefPos = poseSequenceIn->GetPositionAtFrame(referenceFrame, rootbone_idx);

						//Prepare Direction. 2D Trajectory of forward Vector. Hip orientation projected onto ground plane.
						std::vector<glm::vec2> forwardTrajectory = std::vector<glm::vec2>(numSamples);
						glm::quat currentRefRot = animation->mBones[rootbone_idx].GetOrientation(referenceFrame);

						//Prepare Velocity. 2D Trajectory of characters root velocities.
						std::vector<glm::vec2> velTrajectory = std::vector<glm::vec2>(numSamples);
						glm::vec2 currentRefVel = velSeq->GetVelocityAtFrame(referenceFrame, rootbone_idx); //!!!!!

						//Prepare Speed
						std::vector<float> desSpeedTrajectory = std::vector<float>(numSamples, 0.0f);

						//Get Reference Rotation (converted to matrix)
						glm::quat referenceRotation = animation->mBones[rootbone_idx].GetOrientation(referenceFrame);
						glm::quat inverseReferenceRotation = glm::inverse(referenceRotation);
						
						//Define a forwardvector match forward of assimp
						auto forwardBaseVector = glm::vec4(0, 0, 1.0, 0);



						for (int i = 0; i < numSamples; i++) {
							// Positional Trajectory
							glm::vec2 samplePos = poseSequenceIn->GetPositionAtFrame(pastFrameStartIdx + i, rootbone_idx); // x-z as ground plane assumed!?
							posTrajectory[i] = samplePos - curretRefPos;

							//Orientate positional trajectory to face always forward (TEST)
							glm::vec3 temp(posTrajectory[i].x, 0, posTrajectory[i].y);
							temp = inverseReferenceRotation * temp;
							posTrajectory[i] = { temp.x, temp.z };

							//Get matrix from each sample and transform forward
							glm::quat sampleRotation = animation->mBones[rootbone_idx].GetOrientation(pastFrameStartIdx + i);
							//Transform newly generated forward vector with inverse transform of reference rotation
							auto sampleForward = sampleRotation * forwardBaseVector;
							auto relativeSampleForward = inverseReferenceRotation * sampleForward;


							//grab only ground floor components (X&Y ?)
							forwardTrajectory[i] = glm::normalize(glm::vec2(relativeSampleForward.x, relativeSampleForward.z));


							//Velocity
							glm::vec2 cPos = poseSequenceIn->GetPositionAtFrame(pastFrameStartIdx + i, rootbone_idx);
							glm::vec2 pPos = poseSequenceIn->GetPositionAtFrame(glm::max(0, (pastFrameStartIdx + i) -1), rootbone_idx);

							glm::vec2 v = (cPos - pPos) / (1.f / 60.f); //Velociy Unit: cm/s

							auto relativeSampleVelocity = inverseReferenceRotation * glm::vec3(v.x, 0.0, v.y);

							velTrajectory[i] = glm::vec2(relativeSampleVelocity.x, relativeSampleVelocity.z) / 100.f; //Velocity Unit: m/s

							//Speed
							desSpeedTrajectory[i] = glm::length(velTrajectory[i]);
						}

						//Flatten Root Trajectory data into single vector.
						std::vector<float> flatTrajectoryData;

						for (int i = 0; i < numSamples; i++) {
							flatTrajectoryData.push_back(posTrajectory[i].x);
							flatTrajectoryData.push_back(posTrajectory[i].y);
							flatTrajectoryData.push_back(forwardTrajectory[i].x);
							flatTrajectoryData.push_back(forwardTrajectory[i].y);
							flatTrajectoryData.push_back(velTrajectory[i].x);
							flatTrajectoryData.push_back(velTrajectory[i].y);
							flatTrajectoryData.push_back(desSpeedTrajectory[i]);
						}

						rootSequenceData.push_back(flatTrajectoryData); // Collect flattened data for whole sequence


						//Current joint positions relative to root position.
						std::vector<glm::vec3> relativeJointPosition = std::vector<glm::vec3>(poseSequenceIn->mPoseSequence[referenceFrame].mPositionData.size());
						glm::vec3 referencePosition = poseSequenceIn->mPoseSequence[referenceFrame - 1].mPositionData[rootbone_idx];

						for (int i = 0; i < poseSequenceIn->mPoseSequence[referenceFrame].mPositionData.size(); i++) {
							glm::vec3 samplePosition = poseSequenceIn->mPoseSequence[referenceFrame].mPositionData[i];

							//TODO skip root / start from specified root
							relativeJointPosition[i] = samplePosition - referencePosition;
						}

						sequenceRelativeJointPosition.push_back(relativeJointPosition);


						// Joint Rotations
						std::vector<glm::quat> relativeJointRotations = std::vector<glm::quat>(skeleton->mNumBones);

						glm::quat referenceJointRotation = animation->mBones[rootbone_idx].GetOrientation(referenceFrame - 1);
						glm::quat inverseReferenceJointRotation = glm::inverse(referenceJointRotation);

						std::vector<glm::mat4> transforms;
						AnimHostHelper::ForwardKinematics(*skeleton, *animation, transforms, referenceFrame);

						for (int i = 0; i < transforms.size(); i++) {
							glm::vec3 scale;
							glm::quat rotation;
							glm::vec3 translation;
							glm::vec3 skew;
							glm::vec4 perspective;

							glm::decompose(transforms[i], scale, rotation, translation, skew, perspective);
							rotation = glm::conjugate(rotation);

							relativeJointRotations[i] = rotation * inverseReferenceJointRotation;
						}

						sequenceRelativJointRotations.push_back(relativeJointRotations);


						// Joint Velocities
						std::vector<glm::vec3> relativeJointVelocities = std::vector<glm::vec3>(velSeq->mJointVelocitySequence[referenceFrame - 1].mJointVelocity.size());

						for (int i = 0; i < relativeJointVelocities.size(); i++) {
							relativeJointVelocities[i] = inverseReferenceJointRotation * velSeq->mJointVelocitySequence[referenceFrame].mJointVelocity[i];
						}

						sequenceRelativeJointVelocities.push_back(relativeJointVelocities);


///Results Network Output

						/// Position Trajectory
						std::vector<glm::vec2> ouputRootTrajectory;
						glm::vec2 outputRefPos = poseSequenceIn->GetPositionAtFrame(referenceFrame + 1, rootbone_idx);

						//Prepare Velocity
						std::vector<glm::vec2> outputVelTrajectory;
						glm::vec2 outputRefVel = velSeq->GetVelocityAtFrame(referenceFrame + 1, rootbone_idx);

						//Prepare Rotation
						std::vector<glm::vec2> outputFwdTrajectory;
						glm::quat outputRefRot = animation->mBones[rootbone_idx].GetOrientation(referenceFrame + 1);
						glm::quat invOutputRefRot = glm::inverse(outputRefRot);

						for (int i = 6; i < numSamples; i++) {

							// Position
							glm::vec2 samplePos = poseSequenceIn->GetPositionAtFrame(pastFrameStartIdx + 1 + i, rootbone_idx) - outputRefPos;
							glm::vec3 temp(samplePos.x, 0, samplePos.y);
							temp = invOutputRefRot * temp;
							ouputRootTrajectory.push_back({ temp.x, temp.z });

							//Velocity
							glm::vec2 cPos = poseSequenceIn->GetPositionAtFrame(pastFrameStartIdx + i + 1, rootbone_idx);
							glm::vec2 pPos = poseSequenceIn->GetPositionAtFrame(glm::max(0, (pastFrameStartIdx + i + 1) - 1), rootbone_idx);
							glm::vec2 v = (cPos - pPos) / (1.f / 60.f ); 
							auto relativeSampleVelocity = invOutputRefRot * glm::vec3(v.x, 0.0, v.y);
							outputVelTrajectory.push_back(glm::vec2(relativeSampleVelocity.x, relativeSampleVelocity.z ) / 100.f);// Velociy Unit: m/s

							//Direction
							glm::quat sampleRotation = animation->mBones[rootbone_idx].GetOrientation(pastFrameStartIdx + 1 + i);
							auto sampleForward = sampleRotation * forwardBaseVector;
							auto relativeSampleForward = invOutputRefRot * sampleForward;
							outputFwdTrajectory.push_back({ relativeSampleForward.x, relativeSampleForward.z });
						}

						//Flatten Root Trajectory data into single vector.
						std::vector<float> outFlatTrajectoryData;

						for (int i = 0; i < ouputRootTrajectory.size(); i++) {
							outFlatTrajectoryData.push_back(ouputRootTrajectory[i].x);
							outFlatTrajectoryData.push_back(ouputRootTrajectory[i].y);
							outFlatTrajectoryData.push_back(outputFwdTrajectory[i].x);
							outFlatTrajectoryData.push_back(outputFwdTrajectory[i].y);
							outFlatTrajectoryData.push_back(outputVelTrajectory[i].x);
							outFlatTrajectoryData.push_back(outputVelTrajectory[i].y);
						}

						Y_RootSequenceData.push_back(outFlatTrajectoryData); // Collect flattened data for whole sequence

						//Output Joint Positions
						std::vector<glm::vec3> OutputJointPosition = std::vector<glm::vec3>(poseSequenceIn->mPoseSequence[referenceFrame].mPositionData.size());
						glm::vec3 outputReferencePosition = poseSequenceIn->mPoseSequence[referenceFrame].mPositionData[rootbone_idx];

						//todo only children
						for (int i = 0; i < poseSequenceIn->mPoseSequence[referenceFrame].mPositionData.size(); i++) {
							glm::vec3 samplePosition = poseSequenceIn->mPoseSequence[referenceFrame].mPositionData[i];

							//TODO skip root / start from specified root
							OutputJointPosition[i] = samplePosition - outputReferencePosition;
						}
						Y_SequenceRelativeJointPosition.push_back(OutputJointPosition);

						//Output Joint Rotations

						std::vector<glm::quat> OutputJointRotations = std::vector<glm::quat>(skeleton->mNumBones);

						glm::quat outputRefJointRotation = animation->mBones[rootbone_idx].GetOrientation(referenceFrame);
						glm::quat invOutputRefJointRotation = glm::inverse(outputRefJointRotation);

						std::vector<glm::mat4> OutputTransforms;
						AnimHostHelper::ForwardKinematics(*skeleton, *animation, transforms, referenceFrame);

						for (int i = 0; i < transforms.size(); i++) {
							glm::vec3 scale;
							glm::quat rotation;
							glm::vec3 translation;
							glm::vec3 skew;
							glm::vec4 perspective;

							glm::decompose(transforms[i], scale, rotation, translation, skew, perspective);
							rotation = glm::conjugate(rotation);

							OutputJointRotations[i] = rotation * invOutputRefJointRotation;
						}

						Y_SequenceRelativJointRotations.push_back(OutputJointRotations);

						//Output Joint Velocities
						std::vector<glm::vec3> OutputJointVelocities = std::vector<glm::vec3>(velSeq->mJointVelocitySequence[referenceFrame].mJointVelocity.size());

						for (int i = 0; i < relativeJointVelocities.size(); i++) {
							OutputJointVelocities[i] = invOutputRefJointRotation * velSeq->mJointVelocitySequence[referenceFrame].mJointVelocity[i];
						}

						Y_SequenceRelativeJointVelocities.push_back(OutputJointVelocities);

						//Get Reference Rotation (converted to matrix)
						glm::quat nextFrameRotation = animation->mBones[rootbone_idx].GetOrientation(referenceFrame + 1);
						//glm::quat inverseReferenceRotation = glm::inverse(referenceRotation);
						glm::quat testRot = inverseReferenceRotation * nextFrameRotation;

						auto angle = glm::orientedAngle({ 0.0, 1.0f }, forwardTrajectory[7]);

						glm::vec2 testPos = outputRefPos - curretRefPos;

						glm::vec3 relativePos = inverseReferenceRotation* glm::vec3(testPos.x , 0.0 , testPos.y);
						testPos = { relativePos.x, relativePos.z };

						glm::vec3 delta = { testPos, angle };
						Y_SequenceDeltaUpdate.push_back(delta);
					}
					

//Data Write
					if (bOverwriteDataExport) {
						//Delete Existing Files
						FileHandler<QDataStream>::deleteFile(exportDirectory + metadataFileName);
						FileHandler<QDataStream>::deleteFile(exportDirectory + sequencesFileName);
						FileHandler<QDataStream>::deleteFile(exportDirectory + dataXFileName);
						FileHandler<QDataStream>::deleteFile(exportDirectory + dataYFileName);

						bOverwriteDataExport = false;
						_cbOverwrite->setCheckState(Qt::Unchecked);

					}
					//proccessVelocityData(velSeq, animation);

					writeMetaData();
					writeInputData();
					writeOutputData();

					

				}
			}
		}
	}
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

						header += ",jrot_x_" + boneName;
						header += ",jrot_y_" + boneName;
						header += ",jrot_z_" + boneName;
						header += ",jrot_W_" + boneName;

						header += ",jvel_x_" + boneName;
						header += ",jvel_y_" + boneName;
						header += ",jvel_z_" + boneName;

						featureCount += 10;

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
					for (int i = 6; i < numSamples; i++) {
						header += ",out_root_pos_x_" + QString::number(i);
						header += ",out_root_pos_y_" + QString::number(i);
						header += ",out_root_fwd_x_" + QString::number(i);
						header += ",out_root_fwd_y_" + QString::number(i);
						header += ",out_root_vel_x_" + QString::number(i);
						header += ",out_root_vel_y_" + QString::number(i);

						featureCount += 6;
					};

					qDebug() << poseSequenceIn->mPoseSequence[0].mPositionData.size() << " and bone size:" << skeleton->mNumBones;
					for (int i = 0; i < poseSequenceIn->mPoseSequence[0].mPositionData.size(); i++) {
						QString boneName = QString::fromStdString(skeleton->bone_names_reverse.at(i));
						header += ",out_jpos_x_" + boneName;
						header += ",out_jpos_y_" + boneName;
						header += ",out_jpos_z_" + boneName;

						header += ",out_jrot_x_" + boneName;
						header += ",out_jrot_y_" + boneName;
						header += ",out_jrot_z_" + boneName;
						header += ",out_jrot_W_" + boneName;

						header += ",out_jvel_x_" + boneName;
						header += ",out_jvel_y_" + boneName;
						header += ",out_jvel_z_" + boneName;

						featureCount += 10;

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
					int countFeatures = (rootSequenceData[0].size() + (poseSequenceIn->mPoseSequence[0].mPositionData.size() * 10));
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

							flattenedOutput[fltIdx + 3] = sequenceRelativJointRotations[idx][i].x;
							flattenedOutput[fltIdx + 4] = sequenceRelativJointRotations[idx][i].y;
							flattenedOutput[fltIdx + 5] = sequenceRelativJointRotations[idx][i].z;
							flattenedOutput[fltIdx + 6] = sequenceRelativJointRotations[idx][i].w;

							flattenedOutput[fltIdx + 7] = sequenceRelativeJointVelocities[idx][i].x;
							flattenedOutput[fltIdx + 8] = sequenceRelativeJointVelocities[idx][i].y;
							flattenedOutput[fltIdx + 9] = sequenceRelativeJointVelocities[idx][i].z;

							fltIdx += 10;
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
					int countFeatures = Y_RootSequenceData.size() * ( 3 + Y_RootSequenceData[0].size() + (poseSequenceIn->mPoseSequence[0].mPositionData.size() * 10));
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

							flattenedOutput[fltIdx + 3] = Y_SequenceRelativJointRotations[idx][i].x;
							flattenedOutput[fltIdx + 4] = Y_SequenceRelativJointRotations[idx][i].y;
							flattenedOutput[fltIdx + 5] = Y_SequenceRelativJointRotations[idx][i].z;
							flattenedOutput[fltIdx + 6] = Y_SequenceRelativJointRotations[idx][i].w;

							flattenedOutput[fltIdx + 7] = Y_SequenceRelativeJointVelocities[idx][i].x;
							flattenedOutput[fltIdx + 8] = Y_SequenceRelativeJointVelocities[idx][i].y;
							flattenedOutput[fltIdx + 9] = Y_SequenceRelativeJointVelocities[idx][i].z;
							
							fltIdx += 10;
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


