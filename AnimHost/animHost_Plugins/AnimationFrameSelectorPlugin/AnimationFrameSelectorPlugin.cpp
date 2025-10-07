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

 

#include "AnimationFrameSelectorPlugin.h"
#include <QtWidgets>

AnimationFrameSelectorPlugin::AnimationFrameSelectorPlugin()
{
    _widget = nullptr;
    _slider = nullptr;

    _animationOut = std::make_shared<AnimNodeData<Animation>>();

}

AnimationFrameSelectorPlugin::~AnimationFrameSelectorPlugin()
{
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
    onFrameChange(0);
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
        _slider->setTickInterval(duration/3);

        _l2->setText(QString::number((_slider->maximum() / 3) * 1));
        _l3->setText(QString::number((_slider->maximum() / 3) * 2));
        _l4->setText(QString::number(_slider->maximum()));

    }
    else {
        return;
    }

    qDebug() << "AnimationFrameSelectorPlugin setInData";
}

bool AnimationFrameSelectorPlugin::isDataAvailable() {
    return !_animationIn.expired();
}

QWidget* AnimationFrameSelectorPlugin::embeddedWidget()
{
	if (!_widget) {

        _widget = new QWidget();

        _slider = new QSlider(Qt::Horizontal);
        _slider->setTickInterval(_slider->maximum() / 3);
        _slider->setTickPosition(QSlider::TicksBelow);
        _slider->setTracking(false);

        _l1 = new QLabel("0");
        _l2 = new QLabel(QString::number((_slider->maximum() / 3)*1));
        _l3 = new QLabel(QString::number((_slider->maximum() / 3)*2));
        _l4 = new QLabel(QString::number(_slider->maximum()));

        _l1->setAlignment(Qt::AlignCenter);
        _l2->setAlignment(Qt::AlignCenter);
        _l3->setAlignment(Qt::AlignCenter);
        _l4->setAlignment(Qt::AlignCenter);

        QGridLayout* layout = new QGridLayout;

        layout->addWidget(_slider, 0, 1, 1, 6);
        layout->addWidget(_l1, 1, 0, 1, 2,Qt::AlignCenter);
        layout->addWidget(_l2, 1, 2, 1, 2,Qt::AlignCenter);
        layout->addWidget(_l3, 1, 4, 1, 2,Qt::AlignCenter);
        layout->addWidget(_l4, 1, 6, 1, 2,Qt::AlignCenter);

        _widget->setLayout(layout);

        connect(_slider, &QSlider::valueChanged, this, &AnimationFrameSelectorPlugin::onFrameChange);
	}

    _widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
        "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
        "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
        "QLabel{padding: 5px;}"
    );

	return _widget;
}

void AnimationFrameSelectorPlugin::onFrameChange(int value)
{
    qDebug() << "Selected Frame " << value;
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
