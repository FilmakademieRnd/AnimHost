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

    presets.push_back({"AH<->Blender Default",
                        glm::inverse(AnimHostHelper::GetCoordinateSystemTransformationMatrix()) * glm::scale(glm::mat4(1.0), glm::vec3(0.01f, 0.01f, 0.01f)),
                        glm::inverse(AnimHostHelper::GetCoordinateSystemTransformationMatrix())
                        });

    presets.push_back({"No Conversion", glm::mat4(1.0), glm::mat4(1.0)});

	presets.push_back({ "Unity", glm::scale(glm::mat4(1.0), glm::vec3(0.01f, 0.01f, 0.01f)), 
                        glm::inverse(AnimHostHelper::GetCoordinateSystemTransformationMatrix()),
                        true});

    activePreset = presets[0];
}

CoordinateConverterPlugin::~CoordinateConverterPlugin()
{

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

   // qDebug() << "CoordinateConverterPlugin setInData";
}

bool CoordinateConverterPlugin::isDataAvailable() {

    if (!_animationIn.expired()) {
        if (auto spAnimationIn = _animationIn.lock()) {
            auto AnimIn = spAnimationIn->getData();
            if (AnimIn->mBones.size() > 0)
                return true;
        }
    }
        return false;
};

void CoordinateConverterPlugin::run()
{
    if (isDataAvailable()) {

        if (auto spAnimationIn = _animationIn.lock()) {
            auto AnimIn = spAnimationIn->getData();
            auto animOut = std::make_shared<Animation>(*AnimIn);
            bool negX, negY, negZ, negW, swapYZ = false;

            negX = xButton->isChecked();
            negY = yButton->isChecked();
            negZ = zButton->isChecked();
            negW = wButton->isChecked();
            swapYZ = swapYzButton->isChecked();

            // Apply Transforms to Character Object Root

            for (int i = 0; i < animOut->mBones[0].mPositonKeys.size(); i++) {
                
				glm::vec3 pos = animOut->mBones[0].mPositonKeys[i].position;
				glm::quat rot = animOut->mBones[0].mRotationKeys[i].orientation;

				glm::mat4 transform = glm::translate(glm::mat4(1.0), pos) * glm::toMat4(rot);

				// Transform to target system
				transform = activePreset.transformMatrix * transform * glm::inverse(activePreset.transformMatrix);

				// Extract position and rotation
				animOut->mBones[0].mPositonKeys[i].position = glm::vec3(transform[3]);
				animOut->mBones[0].mRotationKeys[i].orientation = glm::toQuat(transform);

            }


			// Revert Asset specific conversion applied to character "root" bone (Usually Hip)
            glm::mat4 conversion = activePreset.characterRootTransform;
            for (int i = 0; i < animOut->mBones[1].mPositonKeys.size(); i++) {

                glm::vec3 pos = glm::vec4(animOut->mBones[1].mPositonKeys[i].position, 1.f);
                glm::quat rot = animOut->mBones[1].mRotationKeys[i].orientation  ;

                glm::mat4 transform =glm::translate(glm::mat4(1.0), pos) * glm::toMat4(rot);

                // Transform to target system
                transform = conversion * transform;


                if (activePreset.applyScaleOnCharacter)
                    animOut->mBones[1].mPositonKeys[i].position = glm::vec3(transform[3]) / 100.f;
                else
					animOut->mBones[1].mPositonKeys[i].position = glm::vec3(transform[3]);


                animOut->mBones[1].mRotationKeys[i].orientation = glm::toQuat(transform);

            }



			//Apply bone specific transformations to the rest of the bones
            for (int i = 0; i < animOut->mBones.size(); i++) {

				//Iterate over T, R, S Keys individually, might contain different amount of keys
                int numKeys = animOut->mBones[i].mNumKeysRotation;
                for (int j = 0; j < numKeys; j++) {
                    animOut->mBones[i].mRotationKeys[j].orientation = ConvertToTargetSystem(animOut->mBones[i].mRotationKeys[j].orientation,
                        swapYZ, negX, negY, negZ, negW);
                }

                numKeys = animOut->mBones[i].mNumKeysPosition;
                for (int j = 0; j < numKeys; j++) {
                    animOut->mBones[i].mPositonKeys[j].position = ConvertToTargetSystem(animOut->mBones[i].mPositonKeys[j].position,
                        swapYZ, negX, negY, negZ, negW)  ;


					// Fill other position keys with rest translation, no animation expected but prevent empty keys
                    if (i > 1)
                        animOut->mBones[i].mPositonKeys[j].position = animOut->mBones[i].mRestingTransform[3] / 100.f;
                }

                numKeys = animOut->mBones[i].mNumKeysScale;
                for (int j = 0; j < numKeys; j++) {
                    animOut->mBones[i].mScaleKeys[j].scale = ConvertToTargetSystem(animOut->mBones[i].GetScale(j),
                        swapYZ, negX, negY, negZ, negW);
                }

                animOut->mBones[i].restingRotation = ConvertToTargetSystem(animOut->mBones[i].restingRotation,
                    swapYZ, negX, negY, negZ, negW);

                animOut->mBones[i].mRestingTransform = ConvertToTargetSystem(animOut->mBones[i].mRestingTransform,
                    swapYZ, negX, negY, negZ, negW);
            }

            _animationOut->setVariant(QVariant::fromValue(animOut));

            emitDataUpdate(0);
            emitRunNextNode();
        }
        else {

            emitDataInvalidated(0);
        }

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
        widget = new QWidget();

        //preset selection
        presetLayout = new QHBoxLayout();
        presetLabel = new QLabel("Presets:");
        presetComboBox = new QComboBox();

        for (auto& preset : presets) {
			presetComboBox->addItem(preset.name);
		}

        presetLayout->addWidget(presetLabel);
        presetLayout->addWidget(presetComboBox);

        debugCheckBox = new QCheckBox("Enable Debug Mode");

        _layout = new QVBoxLayout();

        _layout->addLayout(presetLayout);
        _layout->addWidget(debugCheckBox);

        //Debug Widget
        {
            debugWidget = new QWidget(widget);
            debugLayout = new QVBoxLayout();

            // Quat Checkboxes
            xButton = new QCheckBox("- X");
            yButton = new QCheckBox("- Y");
            zButton = new QCheckBox("- Z");
            wButton = new QCheckBox("- W");
            swapYzButton = new QCheckBox("Swap Y-Z");

            debugLayout->addWidget(xButton);
            debugLayout->addWidget(yButton);
            debugLayout->addWidget(zButton);
            debugLayout->addWidget(wButton);
            debugLayout->addWidget(swapYzButton);

            // Matrix Editor
            matrixLabel = new QLabel("Transformation Character Root Matrix:");
            matrixEditor = new MatrixEditorWidget();
            matrixEditor->SetMatrix(activePreset.transformMatrix);
            applyButton = new QPushButton("Apply Matrix");

            debugLayout->addWidget(matrixLabel);
            debugLayout->addWidget(matrixEditor);
            debugLayout->addWidget(applyButton);

            debugWidget->setLayout(debugLayout);
            debugWidget->setVisible(false);

            _layout->addWidget(debugWidget);
        }
        
        widget->setLayout(_layout);

        QObject::connect(debugCheckBox, &QCheckBox::toggled, [=](bool checked) {
			debugWidget->setVisible(checked);

            debugWidget->parentWidget()->adjustSize();
    
            Q_EMIT embeddedWidgetSizeUpdated();
		});

        QObject::connect(presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
            const QSignalBlocker blocker(xButton);
            const QSignalBlocker blocker2(yButton);
            const QSignalBlocker blocker3(zButton);
            const QSignalBlocker blocker4(wButton);
            const QSignalBlocker blocker5(swapYzButton);
            
            activePreset = presets[index];
			xButton->setChecked(activePreset.negX);
			yButton->setChecked(activePreset.negY);
			zButton->setChecked(activePreset.negZ);
			wButton->setChecked(activePreset.negW);
			swapYzButton->setChecked(activePreset.flipYZ);
            matrixEditor->SetMatrix(activePreset.transformMatrix);

			run();
		});

        QObject::connect(xButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);
        QObject::connect(yButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);
        QObject::connect(zButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);
        QObject::connect(wButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);
        QObject::connect(swapYzButton, &QCheckBox::stateChanged, this, &CoordinateConverterPlugin::onChangedCheck);

        QObject::connect(applyButton, &QPushButton::clicked, this, [=]() {
            glm::mat4 mat = this->matrixEditor->GetMatrix();
            activePreset.transformMatrix = mat;
            run();
        });
	}

    widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
           "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
           "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
           "QLabel{padding: 0px;}"
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
    qDebug() << "Quaternion Swizzel";
    run();
}
