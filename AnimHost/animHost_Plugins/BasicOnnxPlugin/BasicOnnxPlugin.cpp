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

 

#include "BasicOnnxPlugin.h"
#include "OnnxHelper.h"
#include "animhosthelper.h"
#include <QPushButton>
#include <QtWidgets>
#include "OnnxModelViewWidget.h"
#include "OnnxTensor.h"
#include <onnxruntime_cxx_api.h>
#include <nodedatatypes.h>



BasicOnnxPlugin::BasicOnnxPlugin()
{

    widget = nullptr;

    _onnxModel = nullptr;

    qRegisterMetaType<std::shared_ptr<OnnxTensor>>("Tensor");


    AnimNodeData<OnnxTensor> test;

    qDebug() << "BasicOnnxPlugin created";
}

BasicOnnxPlugin::~BasicOnnxPlugin() 
{
    qDebug() << "~BasicOnnxPlugin()";
}

unsigned int BasicOnnxPlugin::nDataPorts(QtNodes::PortType portType) const
{
    if (_onnxModel) {
        if (portType == QtNodes::PortType::In)
            return _dataIn.size();
        else
            return _dataOut.size();
    }
    else {
        return 0;
    }
}

NodeDataType BasicOnnxPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        
        return AnimNodeData<OnnxTensor>::staticType();
    else
        return AnimNodeData<OnnxTensor>::staticType();
}

std::shared_ptr<NodeData> BasicOnnxPlugin::processOutData(QtNodes::PortIndex port)
{
    return nullptr;
}

void BasicOnnxPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "BasicOnnxPlugin setInData";
}

bool BasicOnnxPlugin::isDataAvailable() {
    bool availableData = true;
    for (auto& data : _dataIn) {
        availableData = availableData && (!data.expired());
    }
    return availableData;
}

void BasicOnnxPlugin::run() {

}

QWidget* BasicOnnxPlugin::embeddedWidget()
{
	if (!widget) {
        widget = new OnnxModelViewWidget();
        auto b = widget->GetButton();
        connect(b, &QPushButton::released, this, &BasicOnnxPlugin::onButtonClicked);
        widget->setMinimumHeight(widget->sizeHint().height());
        widget->setMaximumWidth(widget->sizeHint().width());

       




	}
	return widget;
}

QString BasicOnnxPlugin::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{

    int pIdx = portIndex - 1;
    if (pIdx == -1)
        return QString("Run");

    if (_onnxModel) {
        std::string s;
        if (portType == QtNodes::PortType::In) {

            s = _onnxModel->GetTensorNames()[pIdx]+ " ";
            s += _onnxModel->GetTensorShapes()[pIdx];
            return s.c_str();
        }
        else {
            s = _onnxModel->GetTensorNames(false)[pIdx] + " ";
            s += _onnxModel->GetTensorShapes(false)[pIdx];
            return s.c_str();
        }
    }

    return QString();
}

void BasicOnnxPlugin::LoadOnnxModel()
{
    qDebug() << "Onnx Model Exploration";

    clearPorts();

    _onnxModel = std::make_unique<OnnxModel>();

    _onnxModel->LoadOnnxModel(OnnxModelFilePath);


    widget->UpdateModelDescription(_onnxModel->GetTensorNames(), _onnxModel->GetTensorShapes());
    widget->UpdateModelDescription(_onnxModel->GetTensorNames(false), _onnxModel->GetTensorShapes(false), false);

    Q_EMIT embeddedWidgetSizeUpdated();

    addPort();

    Q_EMIT embeddedWidgetSizeUpdated();

    RunDummyInference();

    
}

void BasicOnnxPlugin::RunDummyInference()
{
    _onnxModel->RunInference();

}

void BasicOnnxPlugin::clearPorts()
{
    //ToDo
    int numIn = this->nPorts(QtNodes::PortType::In);

    for (int i = 0; i < numIn; i++){
        this->portsAboutToBeDeleted(QtNodes::PortType::In, i, i);
    }

    int numOut = this->nPorts(QtNodes::PortType::Out);

    for (int i = 0; i < numIn; i++) {
        this->portsAboutToBeDeleted(QtNodes::PortType::Out, i, i);
    }

    //ToDO clear tensor connection to session.
   
    _dataIn.clear();
    _dataOut.clear();


    Q_EMIT portsDeleted();

}

void BasicOnnxPlugin::addPort()
{
    if (_onnxModel) {
        
        int numIn = _onnxModel->GetNumTensors();
        for (int i = 0; i < numIn; i++) {
            this->portsAboutToBeInserted(QtNodes::PortType::In, i, i);
            _dataIn.emplace_back(std::weak_ptr<QtNodes::NodeData>());  
        }

        int numOut = _onnxModel->GetNumTensors(false);
        for (int i = 0; i < numOut; i++) {
            this->portsAboutToBeInserted(QtNodes::PortType::Out, i, i);
            _dataOut.emplace_back(std::shared_ptr<QtNodes::NodeData>());
        }

        Q_EMIT portsInserted();
    }
}



void BasicOnnxPlugin::onButtonClicked()
{
    qDebug() << "Onnx Model Button Clicked";

    bModelDescValid = false;
    QString file_name = QFileDialog::getOpenFileName(nullptr, "Open Onnx Model", "C://", "(*.onnx)");
    OnnxModelFilePath = file_name;
    

    LoadOnnxModel();

    qDebug() << file_name;
    QString shorty = AnimHostHelper::shortenFilePath(file_name, 10);

    widget->label->setText(shorty);
    Q_EMIT embeddedWidgetSizeUpdated();
    Q_EMIT dataUpdated(0);

}
