
#include "AnimationFrameSelectorPlugin.h"
#include <QPushButton>

AnimationFrameSelectorPlugin::AnimationFrameSelectorPlugin()
{
    _widget = nullptr;
    _slider = nullptr;

    _animationOut = std::make_shared<AnimNodeData<Animation>>();

    qDebug() << "AnimationFrameSelectorPlugin created";
}

AnimationFrameSelectorPlugin::~AnimationFrameSelectorPlugin()
{
    qDebug() << "~AnimationFrameSelectorPlugin()";
}

unsigned int AnimationFrameSelectorPlugin::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 1;
    else
        return 1;
}

NodeDataType AnimationFrameSelectorPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return AnimNodeData<Animation>::staticType();
    else
        return AnimNodeData<Animation>::staticType();
}



void AnimationFrameSelectorPlugin::run()
{
    qDebug() << this->name() << " RUN";
}

std::shared_ptr<NodeData> AnimationFrameSelectorPlugin::processOutData(QtNodes::PortIndex port)
{
    return _animationOut;
}

void AnimationFrameSelectorPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    if (!data) {
        Q_EMIT dataInvalidated(0);
    }
    _animationIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
    if (auto spAnimation = _animationIn.lock()) {

        int duration = spAnimation->getData()->mDurationFrames;
        _slider->setMaximum(duration - 1);

    }
    else {
        return;
    }

    qDebug() << "AnimationFrameSelectorPlugin setInData";
}

QWidget* AnimationFrameSelectorPlugin::embeddedWidget()
{
	if (!_widget) {

        _widget = new QWidget();

        _slider = new QSlider(Qt::Horizontal, _widget);
        _slider->setTracking(false);

        connect(_slider, &QSlider::valueChanged, this, &AnimationFrameSelectorPlugin::onFrameChange);
	}

	return _widget;
}

void AnimationFrameSelectorPlugin::onFrameChange(int value)
{
    qDebug() << value;
    auto AnimOut = _animationOut->getData();
    AnimOut->mDurationFrames = 1;

    if (auto spAnimationIn = _animationIn.lock()) {
        
        auto AnimIn = spAnimationIn->getData();
        int numBones = AnimIn->mBones.size();

        AnimOut->mBones.resize(numBones);

        for (int i = 0; i < numBones; i++) {

            AnimOut->mBones[i] = Bone(AnimIn->mBones[i], value);
        }

        emitDataUpdate(0);

        emitRunNextNode();
    }
    else {
        emitDataInvalidated(0);
    }

}
