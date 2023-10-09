
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

    qDebug() << "DataExportPlugin created";
}

DataExportPlugin::~DataExportPlugin()
{
    qDebug() << "~DataExportPlugin()";
}

QJsonObject DataExportPlugin::save() const
{
    QJsonObject nodeJson = NodeDelegateModel::save();

    nodeJson["dir"] = exportDirectory;

    nodeJson["writeBin"] = bWriteBinaryData;

    nodeJson["overwrite"] = bOverwritePoseSeq;

    return nodeJson;
}

void DataExportPlugin::load(QJsonObject const& p)
{
    QJsonValue v = p["dir"];

    if (!v.isUndefined()) {
        QString strDir = v.toString();

        if (!strDir.isEmpty()) {
            exportDirectory = strDir;
        }
    }

    v = p["writeBin"];
    if (!v.isUndefined()) {
        bWriteBinaryData = v.toBool();
    }

    v = p["overwrite"];
    if (!v.isUndefined()) {
        bOverwritePoseSeq = v.toBool();
        bOverwriteJointVelSeq = v.toBool();
    }
}

unsigned int DataExportPlugin::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 3;
    else            
        return 0;
}

NodeDataType DataExportPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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

std::shared_ptr<NodeData> DataExportPlugin::processOutData(QtNodes::PortIndex port)
{
    return nullptr;
}

void DataExportPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
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
}

void DataExportPlugin::run()
{
    if (!exportDirectory.isEmpty()) {

        if (auto sp_poseSeq = _poseSequenceIn.lock() && bWritePoseSequence) {
            // Do Stuff
            if (auto sp_skeleton = _skeletonIn.lock()) {
                exportPoseSequenceData();
            }
        }

        if (auto sp_jointVelSeq = _jointVelocitySequenceIn.lock() && bWriteJointVelocity) {

            if (auto sp_skeleton = _skeletonIn.lock()) {
                exportJointVelocitySequence();
            }

        }
    }
}

QWidget* DataExportPlugin::embeddedWidget()
{
    if (!_pushButton) {

        _pushButton = new QPushButton("Select Dir");
        _pushButton->resize(QSize(30, 30));

        _label = new QLabel("Export Path");

        _filePathLayout = new QHBoxLayout();
        _filePathLayout->addWidget(_label);
        _filePathLayout->addWidget(_pushButton);
        _filePathLayout->setSizeConstraint(QLayout::SetMinimumSize);

        _cbWriteBinary = new QCheckBox("Write Binary Data");
        _cbOverwrite = new QCheckBox("Overwrite Existing Data");
        
        _vLayout = new QVBoxLayout();
        _vLayout->addLayout(_filePathLayout);
        _vLayout->addWidget(_cbWriteBinary);
        _vLayout->addWidget(_cbOverwrite);

        widget = new QWidget();
        widget->setLayout(_vLayout);

        connect(_pushButton, &QPushButton::released, this, &DataExportPlugin::onButtonClicked);
        connect(_cbOverwrite, &QCheckBox::stateChanged, this, &DataExportPlugin::onOverrideCheckbox);
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
    exportDirectory = directory + "/";


    QString shorty = AnimHostHelper::shortenFilePath(exportDirectory, 10);
    _label->setText(shorty);
}


void DataExportPlugin::onOverrideCheckbox(int state) {

    bOverwriteJointVelSeq = state;
    bOverwritePoseSeq = state;

}


void DataExportPlugin::exportPoseSequenceData() {
    //Check if Binary or CSV
    bWriteBinaryData = _cbWriteBinary->isChecked();

    if (bWriteBinaryData) {
        writeBinaryPoseSequenceData();
    }
    else {
        writeCSVPoseSequenceData();
    }
}

void DataExportPlugin::writeCSVPoseSequenceData() {

    auto skeletonIn = _skeletonIn.lock()->getData();
    auto poseSequenceIn = _poseSequenceIn.lock()->getData();

    qDebug() << "Write Pose Data to CSV File";


    QFile file(exportDirectory+"pose.csv");

    if (bOverwritePoseSeq) {
        file.open(QIODevice::WriteOnly | QIODevice::Text);   
    }
    else {
        file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    }


    QTextStream out(&file);

    if (bOverwritePoseSeq) {
        out << "seq_id,";
        for (int i = 0; i < skeletonIn->mNumBones; i++) {
            out << QString::fromStdString(skeletonIn->bone_names_reverse.at(i)) << "_x,";
            out << QString::fromStdString(skeletonIn->bone_names_reverse.at(i)) << "_y,";
            out << QString::fromStdString(skeletonIn->bone_names_reverse.at(i)) << "_z";
            if (i != skeletonIn->mNumBones - 1)
                out << ",";
        }
        out << "\n";

        bOverwritePoseSeq = false;
    }

    for (int frame = 0; frame < poseSequenceIn->mPoseSequence.size(); frame++) {
        out << poseSequenceIn->dataSetID << ",";
        for (int bone = 0; bone < skeletonIn->mNumBones; bone++) {
            out << poseSequenceIn->mPoseSequence[frame].mPositionData[bone].x << ",";
            out << poseSequenceIn->mPoseSequence[frame].mPositionData[bone].y << ",";
            out << poseSequenceIn->mPoseSequence[frame].mPositionData[bone].z;
            if (bone != skeletonIn->mNumBones - 1)
                out << ",";

        }
        out << "\n";
    }
}

void DataExportPlugin::writeBinaryPoseSequenceData() {

    auto skeletonIn = _skeletonIn.lock()->getData();
    auto poseSequenceIn = _poseSequenceIn.lock()->getData();

    qDebug() << "Write Pose Data to Binary File";
    
    QFile file(exportDirectory+ "pose.bin");

    if (bOverwritePoseSeq) {
        file.open(QIODevice::WriteOnly);
        bOverwritePoseSeq = false;
        _cbOverwrite->setCheckState(Qt::Unchecked);
    }
    else {
        file.open(QIODevice::WriteOnly | QIODevice::Append);
    }

    QDataStream out(&file);

    int sizeframe = poseSequenceIn->mPoseSequence[0].mPositionData.size() * sizeof(glm::vec3);

    for (int frame = 0; frame < poseSequenceIn->mPoseSequence.size(); frame++) {
        out.writeRawData((char*)&poseSequenceIn->mPoseSequence[frame].mPositionData[0], sizeframe);
    }
       
}

//long long option_1(std::size_t bytes)
//{
//    std::vector<uint64_t> data = GenerateData(bytes);
//
//    auto startTime = std::chrono::high_resolution_clock::now();
//    auto myfile = std::fstream("file.binary", std::ios::out | std::ios::binary);
//    myfile.write((char*)&data[0], bytes);
//    myfile.close();


void DataExportPlugin::exportJointVelocitySequence() {
    //Check if Binary or CSV

    bWriteBinaryData = _cbWriteBinary->isChecked();

    if (bWriteBinaryData) {
        writeBinaryJointVelocitySequence();
    }
    else {
        writeCSVJointVelocitySequence();
    }
}

void DataExportPlugin::writeCSVJointVelocitySequence() {

} 

void DataExportPlugin::writeBinaryJointVelocitySequence() {

    auto skeletonIn = _skeletonIn.lock()->getData();
    auto jointVelSeqIn = _jointVelocitySequenceIn.lock()->getData();

    qDebug() << "Write Joint Velocity Data to Binary File";

    QFile file(exportDirectory + "joint_velocity.bin");

    if (bOverwriteJointVelSeq) {
        file.open(QIODevice::WriteOnly);
        bOverwriteJointVelSeq = false;
        _cbOverwrite->setCheckState(Qt::Unchecked);
    }
    else {
        file.open(QIODevice::WriteOnly | QIODevice::Append);
    }

    QDataStream out(&file);

    int sizeframe = jointVelSeqIn->mJointVelocitySequence[0].mJointVelocity.size() * sizeof(glm::vec3);

    for (int frame = 0; frame < jointVelSeqIn->mJointVelocitySequence.size(); frame++) {
        out.writeRawData((char*)&jointVelSeqIn->mJointVelocitySequence[frame].mJointVelocity[0], sizeframe);
    }

}