
#include "GNNPlugin.h"
#include <QPushButton>
#include <animhosthelper.h>

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

GNNPlugin::GNNPlugin()
{
    _pushButton = nullptr;
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
        return 3;
    else            
        return 1;
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

        default:
            return type;
            break;
        }
    else
        return AnimNodeData<Animation>::staticType();;
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
        case 2:
            _jointVelocitySequenceIn.reset();

        default:
            return;
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
    default:
        return;
    }
}

void GNNPlugin::run()
{
    if (auto sp_skeleton = _skeletonIn.lock()) {
        auto skeleton = sp_skeleton->getData();

        if (auto sp_animation = _animationIn.lock()) {
            auto animation = sp_animation->getData();

            if (auto sp_velSeq = _jointVelocitySequenceIn.lock()) {
                auto velSeq = sp_velSeq->getData();



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

                    glm::decompose(transforms[i], scale, rotation, translation, skew, perspective);
                    rotation = glm::conjugate(rotation);

                    controller->initJointPos.push_back(translation);
                    controller->initJointRot.push_back(rotation);

                    controller->initJointVel.push_back(velSeq->mJointVelocitySequence[10].mJointVelocity[i]);
                }


                controller->SetAnimationIn(animation);
                controller->SetSkeleton(skeleton);



                //dummy data
                controller->InitDummyData();

                controller->prepareInput();

                auto animOut = controller->GetAnimationOut();

                _animationOut = std::make_shared<AnimNodeData<Animation>>();
                _animationOut->setData(animOut);

                emitDataUpdate(0);
                emitRunNextNode();

            }
            
        }
    }

    
}

std::shared_ptr<NodeData> GNNPlugin::processOutData(QtNodes::PortIndex port)
{
	return _animationOut;
}

QWidget* GNNPlugin::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &GNNPlugin::onButtonClicked);
	}

	return _pushButton;
}

void GNNPlugin::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}
