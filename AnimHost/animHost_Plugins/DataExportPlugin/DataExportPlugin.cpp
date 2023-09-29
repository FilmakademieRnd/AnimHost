
#include "DataExportPlugin.h"

#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>

#include <iostream>
#include <fstream>

#include "animhosthelper.h"

DataExportPlugin::DataExportPlugin()
{
    
    //Init inputs
    _skeletonIn = std::make_shared<AnimNodeData<Skeleton>>();
    _poseSequenceIn = std::make_shared<AnimNodeData<PoseSequence>>();
    _jointVelocitySequenceIn = std::make_shared<AnimNodeData<JointVelocitySequence>>();
    
    
    widget = nullptr;
    _pushButton = nullptr;
    _label = nullptr;
    _filePathLayout = nullptr;

    qDebug() << "DataExportPlugin created";
}

DataExportPlugin::~DataExportPlugin()
{
    qDebug() << "~DataExportPlugin()";
}

unsigned int DataExportPlugin::nPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 3;
    else            
        return 0;
}

NodeDataType DataExportPlugin::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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

        default:
            return type;
        }
    else
        return type;
}

void DataExportPlugin::setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "DataExportPlugin setInData";

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
        _poseSequenceIn = std::static_pointer_cast<AnimNodeData<PoseSequence>>(data);
        break;
    case 2:
        _jointVelocitySequenceIn = std::static_pointer_cast<AnimNodeData<JointVelocitySequence>>(data);
        break;

    default:
        return;
    }

    // Write Data

    if (auto sp_poseSeq = _poseSequenceIn.lock() && bWritePoseSequence) {
        // Do Stuff
        if (auto sp_skeleton = _skeletonIn.lock()) {
            exportPoseSequenceData();
        }
    }

    if (auto sp_jointVelSeq = _jointVelocitySequenceIn.lock() && bWriteJointVelocity) {
        // Do Stuff
    }

}

std::shared_ptr<NodeData> DataExportPlugin::outData(QtNodes::PortIndex port)
{
	return nullptr;
}

QWidget* DataExportPlugin::embeddedWidget()
{
    if (!_pushButton) {
        _pushButton = new QPushButton("Select Dir");
        _label = new QLabel("Export Path");

        _pushButton->resize(QSize(30, 30));
        _filePathLayout = new QHBoxLayout();

        _filePathLayout->addWidget(_label);
        _filePathLayout->addWidget(_pushButton);

        _filePathLayout->setSizeConstraint(QLayout::SetMinimumSize);

        widget = new QWidget();

        widget->setLayout(_filePathLayout);
        connect(_pushButton, &QPushButton::released, this, &DataExportPlugin::onButtonClicked);
    }

    widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
        "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
        "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
        "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
    );

    return widget;
}

void DataExportPlugin::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";

    QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(nullptr, "Import Animation", "C://"));
    exportDirectory = directory;


    QString shorty = AnimHostHelper::shortenFilePath(exportDirectory, 10);
    _label->setText(shorty);
}



void DataExportPlugin::exportPoseSequenceData() {
    //Check if Binary or CSV
    if (bWriteBinaryData) {
        //Do Stuff
    }
    else {
        writeCSVPoseSequenceData();
    }
}

void DataExportPlugin::writeCSVPoseSequenceData() {

    auto skeletonIn = _skeletonIn.lock()->getData();
    auto poseSequenceIn = _poseSequenceIn.lock()->getData();



    qDebug() << "Write Pose Data to File";

    std::ofstream fileOut("ofgsgargk.csv");

    for (int i = 0; i < skeletonIn->mNumBones; i++) {
        fileOut << skeletonIn->bone_names_reverse.at(i) << "_x,";
        fileOut << skeletonIn->bone_names_reverse.at(i) << "_y,";
        fileOut << skeletonIn->bone_names_reverse.at(i) << "_z";
        if (i != skeletonIn->mNumBones - 1)
            fileOut << ",";
    }
    fileOut << "\n";


    for (int frame = 0; frame < poseSequenceIn->mPoseSequence.size(); frame++) {
        for (int bone = 0; bone < skeletonIn->mNumBones; bone++) {

            fileOut << poseSequenceIn->mPoseSequence[frame].mPositionData[bone].x << ",";
            fileOut << poseSequenceIn->mPoseSequence[frame].mPositionData[bone].y << ",";
            fileOut << poseSequenceIn->mPoseSequence[frame].mPositionData[bone].z;
            if (bone != skeletonIn->mNumBones - 1)
                fileOut << ",";

        }
        fileOut << "\n";
    }

    fileOut.close();

}

void DataExportPlugin::exportJointVelocitySequence() {
    //Check if Binary or CSV
}

void DataExportPlugin::writeCSVJointVelocitySequence() {

} 