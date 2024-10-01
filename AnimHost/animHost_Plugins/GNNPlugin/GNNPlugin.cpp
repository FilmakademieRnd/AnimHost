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

}

GNNPlugin::~GNNPlugin()
{
    qDebug() << "~GNNPlugin()";
}

QJsonObject GNNPlugin::save() const
{
	QJsonObject modelJson = NodeDelegateModel::save();
	if (_fileSelectionWidget) {
		modelJson["fileSelection"] = _fileSelectionWidget->GetSelectedDirectory();
	}
	return modelJson;
}

void GNNPlugin::load(QJsonObject const& p)
{
	QJsonValue v = p["fileSelection"];
    if (!v.isUndefined()) {
		_NetworkPath = v.toString();
        if (!_NetworkPath.isEmpty()) {
            _fileSelectionWidget->SetDirectory(_NetworkPath);

            _widget->adjustSize();
            _widget->updateGeometry();
        }
	}
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

bool GNNPlugin::isDataAvailable() {
    return _skeletonIn.lock() && _animationIn.lock() && _jointVelocitySequenceIn.lock() && _controlPathIn.lock();
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

                    auto start = std::chrono::high_resolution_clock::now();

                    controller = std::make_unique<GNNController>(_NetworkPath);


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
                    controller->SetMixWeights(_mixRootTranslation->value(), _mixRootRotation->value(), _mixControlPath->value());

                    //dummy data
           
                    controller->prepareInput();

					if (auto animOut = controller->GetAnimationOut()) {
						_animationOut = std::make_shared<AnimNodeData<Animation>>();
						_animationOut->setData(animOut);
                        animOut->mDurationFrames = 1;


                        _debugSignalOut = std::make_shared<AnimNodeData<DebugSignal>>();
                        _debugSignalOut->setData(controller->GetDebugSignal());


                        auto end = std::chrono::high_resolution_clock::now();

                        // Calculate the duration in milliseconds
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                        // Output the duration
                        qDebug() << "Generation took " << duration << " milliseconds to execute.";



                        emitDataUpdate(0);
                        emitDataUpdate(1);
                        emitRunNextNode();
					}
                    else
                    {
						qDebug() << "Error: No Animation generated";

						emitDataInvalidated(0);
                        emitDataInvalidated(1);
                    }

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
       _widget = new QWidget();



       _fileSelectionWidget = new FolderSelectionWidget(_widget, FolderSelectionWidget::File);


       _mixRootRotation = new QDoubleSpinBox(_widget);
       _mixRootRotation->setRange(0.0, 1.0);
       _mixRootRotation->setSingleStep(0.01);
       _mixRootRotation->setValue(0.5);

       _mixRootTranslation = new QDoubleSpinBox(_widget);
       _mixRootTranslation->setRange(0.0, 1.0);
       _mixRootTranslation->setSingleStep(0.01);
       _mixRootTranslation->setValue(0.5);

       _mixControlPath = new QDoubleSpinBox(_widget);
       _mixControlPath->setRange(0.0, 5.0);
       _mixControlPath->setSingleStep(0.1);
       _mixControlPath->setValue(1.0);    
       
       QVBoxLayout* layout = new QVBoxLayout();
       layout->addWidget(_fileSelectionWidget);

       QGridLayout* gridLayout = new QGridLayout();
       gridLayout->addWidget(new QLabel("Mix Root Rotation"), 0, 0);
       gridLayout->addWidget(_mixRootRotation, 0, 1);
       gridLayout->addWidget(new QLabel("Mix Root Translation"), 1, 0);
       gridLayout->addWidget(_mixRootTranslation, 1, 1);
       gridLayout->addWidget(new QLabel("Mix Control Path"), 2, 0);
       gridLayout->addWidget(_mixControlPath, 2, 1);

       layout->addLayout(gridLayout);

      
       _widget->setLayout(layout);

       connect(_fileSelectionWidget, &FolderSelectionWidget::directoryChanged, this, &GNNPlugin::onFileSelectionChanged);






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


void GNNPlugin::onFileSelectionChanged()
{
    _NetworkPath = _fileSelectionWidget->GetSelectedDirectory();
    Q_EMIT embeddedWidgetSizeUpdated();
}
