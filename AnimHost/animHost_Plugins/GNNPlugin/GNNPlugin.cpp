
#include "GNNPlugin.h"
#include <QPushButton>
#include <animhosthelper.h>
#include <MathUtils.h>

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

GNNPlugin::GNNPlugin()
{
    _widget = nullptr;
    qDebug() << "GNNPlugin created";

    controller = std::make_unique<GNNController>();
}

GNNPlugin::~GNNPlugin()
{
    qDebug() << "~GNNPlugin()";
}

unsigned int GNNPlugin::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 4;
    else            
        return 2;
}

NodeDataType GNNPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        switch (portIndex) {
        case 0:
            return AnimNodeData<Skeleton>::staticType(); 
        case 1:
            return AnimNodeData<Animation>::staticType();
        case 2:
            return AnimNodeData<JointVelocitySequence>::staticType();
        case 3:
            return AnimNodeData<ControlPath>::staticType();

        default:
            return type;
            break;
        }
    else
        switch (portIndex) {
        case 0:
            return AnimNodeData<Animation>::staticType();
        case 1:
            return AnimNodeData<DebugSignal>::staticType();
        default:
            return type;
            break;
        }


}

void GNNPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "GNNPlugin setInData";


    if (!data) {
        switch (portIndex) {
        case 0:
            _skeletonIn.reset();
            break;
        case 1:
            _animationIn.reset();
            break;
        case 2:
            _jointVelocitySequenceIn.reset();
            break;
        case 3:
            _controlPathIn.reset();
            break;
        default:
            break;
        }
        return;
    }


    switch (portIndex) {
    case 0:
        _skeletonIn = std::static_pointer_cast<AnimNodeData<Skeleton>>(data);
        break;
    case 1:
        _animationIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
        break;
    case 2:
        _jointVelocitySequenceIn = std::static_pointer_cast<AnimNodeData<JointVelocitySequence>>(data);
        break;
    case 3:
        _controlPathIn = std::static_pointer_cast<AnimNodeData<ControlPath>>(data);
		break;
    default:
        break;
    }

    return;
}

void GNNPlugin::run()
{
    if (auto sp_skeleton = _skeletonIn.lock()) {
        auto skeleton = sp_skeleton->getData();

        if (auto sp_animation = _animationIn.lock()) {
            auto animation = sp_animation->getData();

            if (auto sp_velSeq = _jointVelocitySequenceIn.lock()) {
                auto velSeq = sp_velSeq->getData();

                if(auto sp_controlPath = _controlPathIn.lock()) {
					auto controlPath = sp_controlPath->getData();




                    //generate dummy joint data

                    std::vector<glm::mat4> transforms;
                    AnimHostHelper::ForwardKinematics(*skeleton, *animation, transforms, 20);

                    controller->initJointPos.clear();
                    controller->initJointRot.clear();
                    controller->initJointVel.clear();

                    for (int i = 0; i < transforms.size(); i++) {
                        glm::vec3 scale;
                        glm::quat rotation;
                        glm::vec3 translation;
                        glm::vec3 skew;
                        glm::vec4 perspective;

                        glm::mat4 root = animation->CalculateRootTransform(20, 0);
                        glm::mat4 relativeTransform = glm::inverse(root) * transforms[i];

                        glm::decompose(relativeTransform, scale, rotation, translation, skew, perspective);

                        controller->initJointPos.push_back(translation);
                        controller->initJointRot.push_back(rotation);

                        controller->initJointVel.push_back(velSeq->mJointVelocitySequence[20].mJointVelocity[i]);
                    }


                    controller->SetAnimationIn(animation);
                    controller->SetSkeleton(skeleton);
                    controller->SetControlPath(controlPath);

                    //dummy data
                    controller->InitDummyData();

                    controller->prepareInput();

                    auto animOut = controller->GetAnimationOut();
                    animOut->mDurationFrames = 1;

                    _animationOut = std::make_shared<AnimNodeData<Animation>>();
                    _animationOut->setData(animOut);


                    _debugSignalOut = std::make_shared<AnimNodeData<DebugSignal>>();
                    _debugSignalOut->setData(controller->GetDebugSignal());



                    emitDataUpdate(0);
                    emitDataUpdate(1);
                    emitRunNextNode();

                }  
            }
        }
    }
 }

std::shared_ptr<NodeData> GNNPlugin::processOutData(QtNodes::PortIndex port)
{
    switch (port) {
    case 0:
        return _animationOut;
    case 1:
        return _debugSignalOut;
    }
}

QWidget* GNNPlugin::embeddedWidget()
{
    if (!_widget) {
       // _widget = new QWidget();
      
    }
    return nullptr;
}

void GNNPlugin::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}
