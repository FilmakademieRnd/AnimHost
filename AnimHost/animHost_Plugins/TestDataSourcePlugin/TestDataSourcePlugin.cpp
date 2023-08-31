#include "TestDataSourcePlugin.h"
#include <iostream>
#include <QFileInfo>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include <fstream>
#include "../../core/commondatatypes.h"

TestDataSourcePlugin::TestDataSourcePlugin()
{
    qDebug() << "Hello Test Data Source Plugin";
 
    //Data
    inputs.append(QMetaType::fromName("Skeleton"));
    inputs.append(QMetaType::fromName("PoseSequence"));

}

TestDataSourcePlugin::~TestDataSourcePlugin()
{
    qDebug() << "~TestDataSourcePlugin()";
}

// execute the main functionality of the plugin
void TestDataSourcePlugin::run(QVariantList in, QVariantList& out)
{
    auto skeletonIn = in[0].value<std::shared_ptr<Skeleton>>();
    auto poseSequenceIn = in[1].value<std::shared_ptr<PoseSequence>>();

    qDebug() << "Eval Test Data Source Plugin";

    std::ofstream fileOut("ok.csv");


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

QString TestDataSourcePlugin::category()
{
    return "Output";
}

QList<QMetaType> TestDataSourcePlugin::inputTypes()
{
    return inputs;
}

QList<QMetaType> TestDataSourcePlugin::outputTypes()
{
    return outputs;
}
