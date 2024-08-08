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

 

#include "CoordinateConverterPlugin.h"
#include <QPushButton>
#include "../../core/commondatatypes.h"
#include "animhosthelper.h"
#include <MathUtils.h>

CoordinateConverterPlugin::CoordinateConverterPlugin()
{
    _animationOut = std::make_shared<AnimNodeData<Animation>>();
    qDebug() << "CoordinateConverterPlugin created";
}

CoordinateConverterPlugin::~CoordinateConverterPlugin()
{
    qDebug() << "~CoordinateConverterPlugin()";
}

unsigned int CoordinateConverterPlugin::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 1;
    else            
        return 1;
}

NodeDataType CoordinateConverterPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return AnimNodeData<Animation>::staticType();
    else
        return AnimNodeData<Animation>::staticType();
}

void CoordinateConverterPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    if (!data) {
        Q_EMIT dataInvalidated(0);
    }

    _animationIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);

    qDebug() << "CoordinateConverterPlugin setInData";
}

bool CoordinateConverterPlugin::isDataAvailable() {
    return !_animationIn.expired();
}

void CoordinateConverterPlugin::run()
{
    if (auto spAnimationIn = _animationIn.lock()) {
        auto AnimIn = spAnimationIn->getData();
        
        auto animOut = std::make_shared<Animation>(*AnimIn);


        bool negX, negY, negZ, negW, swapYZ = false;

        negX = xButton->isChecked();
        negY = yButton->isChecked();
        negZ = zButton->isChecked();
        negW = wButton->isChecked();
        swapYZ = swapYzButton->isChecked();
        
      /*  for(int i = 0; i < animOut->mBones.size(); i++) {
            int numKeys = animOut->mBones[i].mNumKeysRotation;
            for (int j = 0; j < numKeys; j++) {
                animOut->mBones[i].mRotationKeys[j].orientation = ConvertToTargetSystem(animOut->mBones[i].mRotationKeys[j].orientation,
                    swapYZ, negX, negY, negZ, negW);
            }

            numKeys = animOut->mBones[i].mNumKeysPosition;
            for (int j = 0; j < numKeys; j++) {
                animOut->mBones[i].mPositonKeys[j].position = ConvertToTargetSystem(animOut->mBones[i].mPositonKeys[j].position,
                    swapYZ, negX, negY, negZ, negW);
            }

            numKeys = animOut->mBones[i].mNumKeysScale;
            for (int j = 0; j < numKeys; j++) {
                animOut->mBones[i].mScaleKeys[j].scale = ConvertToTargetSystem(animOut->mBones[i].mScaleKeys[j].scale,
                    swapYZ, negX, negY, negZ, negW);
            }

            animOut->mBones[i].restingRotation = ConvertToTargetSystem(animOut->mBones[i].restingRotation,
                swapYZ, negX, negY, negZ, negW);

            animOut->mBones[i].mRestingTransform = ConvertToTargetSystem(animOut->mBones[i].mRestingTransform,
                swapYZ, negX, negY, negZ, negW);
        }*/

        //Apply Transform to Root Bone
        for (int i = 0; i < animOut->mBones[1].mPositonKeys.size(); i++) {

            animOut->mBones[1].mRotationKeys[i].orientation = glm::toQuat(glm::inverse(AnimHostHelper::GetCoordinateSystemTransformationMatrix())) * animOut->mBones[0].mRotationKeys[i].orientation * animOut->mBones[1].mRotationKeys[i].orientation;
            animOut->mBones[1].mPositonKeys[i].position = glm::toQuat(glm::inverse(AnimHostHelper::GetCoordinateSystemTransformationMatrix())) * glm::vec3(animOut->mBones[1].mPositonKeys[i].position) ;
        
        }
			
        _animationOut->setVariant(QVariant::fromValue(animOut));

        emitDataUpdate(0);
        emitRunNextNode();
    }
    else {

        emitDataInvalidated(0);
    }

}

std::shared_ptr<NodeData> CoordinateConverterPlugin::processOutData(QtNodes::PortIndex port)
{
	return _animationOut;
}

QWidget* CoordinateConverterPlugin::embeddedWidget()
{
	if (!widget) {
		
        xButton = new QCheckBox("- X");
        yButton = new QCheckBox("- Y");
        zButton = new QCheckBox("- Z");
        wButton = new QCheckBox("- W");
        
        swapYzButton = new QCheckBox("Swap Y-Z");

        _layout = new QVBoxLayout();

        _layout->addWidget(xButton);
        _layout->addWidget(yButton);
        _layout->addWidget(zButton);
        _layout->addWidget(wButton);
        _layout->addWidget(swapYzButton);

        widget = new QWidget();

        widget->setLayout(_layout);

        QObject::connect(xButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);
        QObject::connect(yButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);
        QObject::connect(zButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);
        QObject::connect(wButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);
        QObject::connect(swapYzButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);
	}

    widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
        "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
        "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
        "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
    );

	return widget;
}

glm::quat CoordinateConverterPlugin::ConvertToTargetSystem(const glm::quat& qIn, bool flipYZ, bool negX, bool negY, bool negZ, bool negW)
{
    glm::quat qOut;
    if (!flipYZ) {
        qOut.x = qIn.x * (negX ? -1.0f : 1.0f);
        qOut.y = qIn.y * (negY ? -1.0f : 1.0f);
        qOut.z = qIn.z * (negZ ? -1.0f : 1.0f);
        qOut.w = qIn.w * (negW ? -1.0f : 1.0f);
    }
    else
    {
        qOut.x = qIn.x * (negX ? -1.0f : 1.0f);
        qOut.z = qIn.y * (negY ? -1.0f : 1.0f);
        qOut.y = qIn.z * (negZ ? -1.0f : 1.0f);
        qOut.w = qIn.w * (negW ? -1.0f : 1.0f);
    }

    return qOut;
}


glm::mat4 CoordinateConverterPlugin::ConvertToTargetSystem(const glm::mat4& matIn, bool flipYZ, bool negX, bool negY, bool negZ, bool negW)
{
    glm::mat4 matOut;
    
    glm::mat4 convert;
    
    if (!flipYZ) {
        convert = { (negX ? -1.0 : 1.0),  .0,  .0,  .0,
                           .0, (negY ? -1.0 : 1.0),  .0,  .0,
                           .0,  .0, (negZ ? -1.0 : 1.0),  .0,
                           .0,  .0,  .0, 1.0 };
    }
    else {
        convert = { (negX ? -1.0 : 1.0),  .0,  .0,  .0,
                     .0,   .0, (negY ? -1.0 : 1.0), .0,
                     .0, (negZ ? -1.0 : 1.0),.0,   .0,
                     .0,  .0,  .0, 1.0 };
    }

    matOut = convert * matIn;
    return matOut;
}

glm::vec3 CoordinateConverterPlugin::ConvertToTargetSystem(const glm::vec3& vecIn, bool flipYZ, bool negX, bool negY, bool negZ, bool negW)
{
    glm::vec3 vecOut;

    glm::mat4 convert;

    if (!flipYZ) {
        convert = { (negX ? -1.0 : 1.0),  .0,  .0,  .0,
                           .0, (negY ? -1.0 : 1.0),  .0,  .0,
                           .0,  .0, (negZ ? -1.0 : 1.0),  .0,
                           .0,  .0,  .0, 1.0 };
    }
    else {
        convert = { (negX ? -1.0 : 1.0),  .0,  .0,  .0,
                     .0,   .0, (negY ? -1.0 : 1.0), .0,
                     .0, (negZ ? -1.0 : 1.0),.0,   .0,
                     .0,  .0,  .0, 1.0 };
    }

    vecOut = convert * glm::vec4(vecIn,1);
    return vecOut;
}

void CoordinateConverterPlugin::onChangedCheck(int check)
{
    run();
	qDebug() << "Example Widget Clicked";
}
