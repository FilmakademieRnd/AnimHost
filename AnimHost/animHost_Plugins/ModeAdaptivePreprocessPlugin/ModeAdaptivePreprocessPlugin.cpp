
#define GLM_FORCE_SWIZZLE
#include "ModeAdaptivePreprocessPlugin.h"


#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>

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
    if (portType == QtNodes::PortType::In)
        switch (portIndex) {
        case 0:
            return AnimNodeData<Skeleton>::staticType();
        case 1:
            return AnimNodeData<PoseSequence>::staticType();
        case 2:
            return AnimNodeData<JointVelocitySequence>::staticType();
        case 3:
            return AnimNodeData<Animation>::staticType();
        }
    else
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

    //Build trajectory positions with n frames

    //1st Build Data for one frame
    //2nd expand to multiple frames
    //3rd create valid training data

    int referenceFrame = 14;

    int pastFrameStartIdx = referenceFrame - pastSamples;

    posTrajectory = std::vector<glm::vec2>(numSamples);

    if (auto sp_poseSeq = _poseSequenceIn.lock()) {
        auto poseSequenceIn = sp_poseSeq->getData();

        glm::vec2 curretRefPos = poseSequenceIn->GetRootPositionAtFrame(referenceFrame);

        for (int i = 0; i < numSamples; i++) {
            glm::vec2 sample = poseSequenceIn->GetRootPositionAtFrame(pastFrameStartIdx + i);
            posTrajectory[i] = sample - curretRefPos;
        }
        qDebug() << "Pose Sequence";
    }

    forwardTrajectory = std::vector<glm::vec2>(numSamples);

    if (auto sp_animation = _animationIn.lock()) {
        auto animation = sp_animation->getData();

        glm::quat currentRefRot = animation->mBones[0].GetOrientation(referenceFrame);
        auto frameRT = glm::toMat4(currentRefRot);


        //Forward-Vector
        auto forward = glm::vec4(1.0,  0, 0, 0);
        forward = frameRT * forward;

        glm::vec2 refForward2d = glm::normalize(glm::vec2(forward.x, forward.z));

        for (int i = 0; i < numSamples; i++) {
            glm::quat rot = animation->mBones[0].GetOrientation(pastFrameStartIdx + i);
            auto RT = glm::toMat4(rot);
            auto tmpfwrd = glm::vec4(1.0, 0, 0, 0);
            tmpfwrd = frameRT * tmpfwrd;

            forwardTrajectory[i] = glm::normalize(glm::vec2(tmpfwrd.x, tmpfwrd.z)) - refForward2d;
        }
        qDebug() << "forward";

    }

    velTrajectory = std::vector<glm::vec2>(numSamples);

    if (auto sp_velSeq = _jointVelocitySequenceIn.lock()) {
        auto velSeq = sp_velSeq->getData();

        glm::vec2 currentRefVel = velSeq->GetRootVelocityAtFrame(referenceFrame);

        for (int i = 0; i < numSamples; i++) {
            glm::vec2 sample = velSeq->GetRootVelocityAtFrame(pastFrameStartIdx + i);
            velTrajectory[i] = sample - currentRefVel;
        }

        qDebug() << "velocity";
    }


    desSpeedTrajectory = std::vector<float>(numSamples);

    if (auto sp_velSeq = _jointVelocitySequenceIn.lock()) {
        auto velSeq = sp_velSeq->getData();

        for (int i = 0; i < numSamples; i++) {
            glm::vec2 sample = velSeq->GetRootVelocityAtFrame(pastFrameStartIdx + i);
            desSpeedTrajectory[i] = glm::length(sample);
        }

        qDebug() << "speed";
    }

    if (auto sp_poseSeq = _poseSequenceIn.lock()) {
        auto poseSequenceIn = sp_poseSeq->getData();

        relativeJointPosition = std::vector<glm::vec3>(poseSequenceIn->mPoseSequence[referenceFrame].mPositionData.size());

        for (int i = 0; i < poseSequenceIn->mPoseSequence[referenceFrame].mPositionData.size(); i++) {
            glm::vec3 curretRefPos = poseSequenceIn->mPoseSequence[referenceFrame].mPositionData[i];

            //TODO skip root
      
            relativeJointPosition[i] = curretRefPos;
        }
        qDebug() << "jointpos";
    }

    
    if (auto sp_animation = _animationIn.lock()) {
        auto animation = sp_animation->getData();
        
        if (auto sp_skeleton = _skeletonIn.lock()) {
            
            auto skeleton = sp_skeleton->getData();

            relativeJoitnRotations = std::vector<glm::quat>(skeleton->mNumBones);

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
                relativeJoitnRotations[i] = rotation;
            }
            qDebug() << "jointrot";
        }
    }
    
    if (auto sp_velSeq = _jointVelocitySequenceIn.lock()) {
        auto velSeq = sp_velSeq->getData();
        relativeJointVelocities = std::vector<glm::vec3>(velSeq->mJointVelocitySequence[referenceFrame].mJointVelocity.size());
        for (int i = 0; i < relativeJointVelocities.size(); i++) {
            relativeJointVelocities[i] = velSeq->mJointVelocitySequence[referenceFrame].mJointVelocity[i];
        }
    }


    //Build trajectory directions

    //Build trajectory velocities

    //Build joint positions

    //Build Rotations

    //build velocities

}


void ModeAdaptivePreprocessPlugin::processRelativeRotations() {

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

        QVBoxLayout* layout = new QVBoxLayout();

        layout->addWidget(_boneSelect);

        _widget->setLayout(layout);
        //_widget->setMinimumHeight(_widget->sizeHint().height());
        //_widget->setMaximumWidth(_widget->sizeHint().width());

        connect(_boneSelect, &BoneSelectionWidget::currentBoneChanged, this, &ModeAdaptivePreprocessPlugin::onRootBoneSelectionChanged);

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

void ModeAdaptivePreprocessPlugin::onRootBoneSelectionChanged(const QString& text)
{
	qDebug() << "Root Bone changed: " << text;
}


