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

 

#include "DataExportPlugin.h"

#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>

#include <FileHandler.h>

#include <iostream>
#include <fstream>

#include "animhosthelper.h"

DataExportPlugin::DataExportPlugin()
{
    //Init inputs
    _skeletonIn = std::make_shared<AnimNodeData<Skeleton>>();
    _poseSequenceIn = std::make_shared<AnimNodeData<PoseSequence>>();
    _jointVelocitySequenceIn = std::make_shared<AnimNodeData<JointVelocitySequence>>();
    _validFramesIn = std::make_shared<AnimNodeData<ValidFrames>>();

}

DataExportPlugin::~DataExportPlugin()
{

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

            exportDirectory = exportDirectory;
            QString shorty = AnimHostHelper::shortenFilePath(exportDirectory, 10);
            _label->setText(shorty);
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
        return 4;
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
        case 3:
            return AnimNodeData<ValidFrames>::staticType();

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
        case 3:
            _validFramesIn.reset();
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
    case 3:
        _validFramesIn = std::static_pointer_cast<AnimNodeData<ValidFrames>>(data);
        break;

    default:
        return;
    }
}

bool DataExportPlugin::isDataAvailable() {
    return !_skeletonIn.expired() && !_poseSequenceIn.expired() && !_jointVelocitySequenceIn.expired();
}


void DataExportPlugin::run()
{
    if (!exportDirectory.isEmpty()) {
        if (auto sp_skeleton = _skeletonIn.lock()) {
            writeBinarySkeletonData();

            auto sp_poseSeq = _poseSequenceIn.lock();
            auto sp_jointVelSeq = _jointVelocitySequenceIn.lock();

            // Determine source name and total frames from available data
            QString sourceName;
            int totalFrames = 0;

            if (sp_poseSeq) {
                sourceName = sp_poseSeq->getData()->sourceName;
                totalFrames = sp_poseSeq->getData()->mPoseSequence.size();
            } else if (sp_jointVelSeq) {
                sourceName = sp_jointVelSeq->getData()->sourceName;
                totalFrames = sp_jointVelSeq->getData()->mJointVelocitySequence.size();
            }

            // Get frames to export (filtered by ValidFrames if configured)
            std::vector<int> framesToExport = getFramesToProcess(totalFrames, sourceName);

            if (!framesToExport.empty()) {
                // Segment frames into consecutive groups
                std::vector<std::vector<int>> segments = AnimHostHelper::segmentConsecutiveFrames(framesToExport);

                qDebug() << "[DataExportPlugin] Found" << segments.size()
                         << "consecutive segments to export";

                // Reset index if overwriting
                if (bOverwritePoseSeq || bOverwriteJointVelSeq) {
                    currentSequenceIndex = 1;
                }

                // Export each segment
                for (size_t i = 0; i < segments.size(); i++) {
                    currentFrameSegment = segments[i];
                    isFirstSegment = (i == 0);

                    qDebug() << "[DataExportPlugin] Exporting segment" << (i + 1)
                             << "with" << currentFrameSegment.size()
                             << "frames, index:" << currentSequenceIndex;

                    if (sp_poseSeq && bWritePoseSequence) {
                        exportPoseSequenceData();
                    }

                    if (sp_jointVelSeq && bWriteJointVelocity) {
                        exportJointVelocitySequence();
                    }

                    currentSequenceIndex++;
                }
            }
        }

        // Reset overwrite flags after run to avoid accidental overwriting on next run
        _cbOverwrite->setCheckState(Qt::Unchecked);

        emitRunNextNode();
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

    // Use pre-computed segment from run()
    const std::vector<int>& framesToExport = currentFrameSegment;

    QFile file(exportDirectory+"pose.csv");

    if (isFirstSegment && bOverwritePoseSeq) {
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    else {
        file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    }


    QTextStream out(&file);

    if (isFirstSegment && bOverwritePoseSeq) {
        out << "seq_id,";
        for (int i = 0; i < skeletonIn->mNumBones; i++) {
            out << QString::fromStdString(skeletonIn->bone_names_reverse.at(i)) << "_x,";
            out << QString::fromStdString(skeletonIn->bone_names_reverse.at(i)) << "_y,";
            out << QString::fromStdString(skeletonIn->bone_names_reverse.at(i)) << "_z";
            if (i != skeletonIn->mNumBones - 1)
                out << ",";
        }
        out << "\n";
    }

    // Export only filtered frames using continuous sequence index
    for (int frame : framesToExport) {
        out << currentSequenceIndex << ",";
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

    // Use pre-computed segment from run()
    const std::vector<int>& framesToExport = currentFrameSegment;

    QFile file(exportDirectory+ "pose.bin");

    if (isFirstSegment && bOverwritePoseSeq) {
        file.open(QIODevice::WriteOnly);
    }
    else {
        file.open(QIODevice::WriteOnly | QIODevice::Append);
    }

    QDataStream out(&file);

    int sizeframe = poseSequenceIn->mPoseSequence[0].mPositionData.size() * sizeof(glm::vec3);

    // Export only filtered frames
    for (int frame : framesToExport) {
        out.writeRawData((char*)&poseSequenceIn->mPoseSequence[frame].mPositionData[0], sizeframe);
    }

}

void DataExportPlugin::writeBinarySkeletonData() {
    auto skeletonIn = _skeletonIn.lock()->getData();

    qDebug() << "Write skeleton data to binary file " << sizeof(int);



    QFile file(exportDirectory + "skeleton_data.json");

    file.open(QIODevice::WriteOnly);

    int boneCount = skeletonIn->mNumBones;

    //for(auto iter = skeletonIn->bone_names.begin(); iter != skeletonIn->bone_names.end(); ++iter)
    //{
    //    auto k = iter->first;

    //    qDebug() << k;
    //    //ignore value
    //    //Value v = iter->second;
    //}
    //out.writeRawData((char*)&boneCount, sizeof(int));

    QJsonObject outDataDescription;

    outDataDescription["numBones"] = boneCount;

    QJsonArray boneNames;

    for (const auto& bonePair : skeletonIn->bone_names) {
        
        boneNames.append(QString(bonePair.first.c_str()));
    }

    outDataDescription["boneNames"] = boneNames;

    file.write(QJsonDocument(outDataDescription).toJson());
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

    auto skeletonIn = _skeletonIn.lock()->getData();
    auto jointVelSeqIn = _jointVelocitySequenceIn.lock()->getData();

    qDebug() << "Write Joint Velocity Data to CSV File";

    // Use pre-computed segment from run()
    const std::vector<int>& framesToExport = currentFrameSegment;

    QFile file(exportDirectory + "joint_velocity.csv");

    if (isFirstSegment && bOverwriteJointVelSeq) {
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    else {
        file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    }


    QTextStream out(&file);

    if (isFirstSegment && bOverwriteJointVelSeq) {
        out << "seq_id,";
        out << "frame,";
        for (int i = 0; i < skeletonIn->mNumBones; i++) {
            out << QString::fromStdString(skeletonIn->bone_names_reverse.at(i)) << "_x,";
            out << QString::fromStdString(skeletonIn->bone_names_reverse.at(i)) << "_y,";
            out << QString::fromStdString(skeletonIn->bone_names_reverse.at(i)) << "_z";
            if (i != skeletonIn->mNumBones - 1)
                out << ",";
        }
        out << "\n";
    }

    // Export only filtered frames using continuous sequence index
    for (int frame : framesToExport) {
        out << currentSequenceIndex << ",";
        out << frame << ",";
        for (int bone = 0; bone < skeletonIn->mNumBones; bone++) {
            out << jointVelSeqIn->mJointVelocitySequence[frame].mJointVelocity[bone].x << ",";
            out << jointVelSeqIn->mJointVelocitySequence[frame].mJointVelocity[bone].y << ",";
            out << jointVelSeqIn->mJointVelocitySequence[frame].mJointVelocity[bone].z;
            if (bone != skeletonIn->mNumBones - 1)
                out << ",";

        }
        out << "\n";
    }
} 

void DataExportPlugin::writeBinaryJointVelocitySequence() {

    auto skeletonIn = _skeletonIn.lock()->getData();
    auto jointVelSeqIn = _jointVelocitySequenceIn.lock()->getData();

    qDebug() << "Write Joint Velocity Data to Binary File";

    // Use pre-computed segment from run()
    const std::vector<int>& framesToExport = currentFrameSegment;

    qDebug() << "[DataExportPlugin] JointVelocitySequence frames to export:" << framesToExport.size();

    QFile file(exportDirectory + "joint_velocity.bin");

    QString fileNameIdent = exportDirectory + "sequences_velocity.txt";

    if (isFirstSegment && bOverwriteJointVelSeq) {
        file.open(QIODevice::WriteOnly);
        FileHandler<QTextStream>::deleteFile(fileNameIdent);
    }
    else {
        file.open(QIODevice::WriteOnly | QIODevice::Append);
    }

    QDataStream out(&file);

    int sizeframe = jointVelSeqIn->mJointVelocitySequence[0].mJointVelocity.size() * sizeof(glm::vec3);

    // Export only filtered frames
    for (int frame : framesToExport) {
        out.writeRawData((char*)&jointVelSeqIn->mJointVelocitySequence[frame].mJointVelocity[0], sizeframe);
    }

    FileHandler<QTextStream> fileIdent = FileHandler<QTextStream>(fileNameIdent);
    QTextStream& outID = fileIdent.getStream();
    QString idString = "";

    // Write sequence identifiers for filtered frames only, using continuous sequence index
    for (int frame : framesToExport) {
        idString += QString::number(currentSequenceIndex) + " ";
        idString += QString::number(frame) + " ";
        idString += "Standard ";
        idString += jointVelSeqIn->sourceName + " ";
        idString += jointVelSeqIn->dataSetID;

        idString += "\n";
        outID << idString;
        idString = "";
    }
}

std::vector<int> DataExportPlugin::getFramesToProcess(int totalFrames, const QString& sourceName)
{
    std::vector<int> frames;

    auto sp_validFrames = _validFramesIn.lock();

    if (!sp_validFrames || sp_validFrames->getData()->isEmpty()) {
        // No Sequences.txt configured - export all frames (current behavior)
        for (int f = 0; f < totalFrames; f++) {
            frames.push_back(f);
        }
        qDebug() << "[DataExportPlugin] No ValidFrames - exporting all"
                 << frames.size() << "frames";
        return frames;
    }

    // ValidFrames is configured - export only valid frames
    auto validFrames = sp_validFrames->getData();
    QString stem = AnimHostHelper::extractFileStem(sourceName);

    if (!validFrames->hasFile(stem)) {
        qWarning() << "[DataExportPlugin] File" << stem
                   << "not found in Sequences.txt - skipping export";
        return frames;  // Empty
    }

    std::vector<int> validFrameList = validFrames->getFrames(stem);
    qDebug() << "[DataExportPlugin] Found" << validFrameList.size()
             << "valid frames for" << stem << "in Sequences.txt";

    // Filter to frames that exist in the data
    for (int f : validFrameList) {
        if (f >= 0 && f < totalFrames) {
            frames.push_back(f);
        }
    }

    qDebug() << "[DataExportPlugin] After bounds check:"
             << frames.size() << "frames to export";
    return frames;
}