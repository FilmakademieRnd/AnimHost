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
    inputs.append(QMetaType::fromName("Pose"));

}

TestDataSourcePlugin::~TestDataSourcePlugin()
{
    qDebug() << "~TestDataSourcePlugin()";
}

// execute the main functionality of the plugin
void TestDataSourcePlugin::run(QVariantList in, QVariantList& out)
{
    //execute
    auto skeletonIn = in[0].value<std::shared_ptr<Skeleton>>();
    auto poseIn = in[1].value<std::shared_ptr<Pose>>();

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

    
        for (int bone = 0; bone < skeletonIn->mNumBones; bone++) {
           
            fileOut << poseIn->mPositionData[bone].x << ",";
            fileOut << poseIn->mPositionData[bone].y << ",";
            fileOut << poseIn->mPositionData[bone].z;
            if (bone != skeletonIn->mNumBones - 1)
                fileOut << ",";

        }
        fileOut << "\n";
  
    fileOut.close();
    




    //out.append(QVariant::fromValue(pose));
}

QString TestDataSourcePlugin::category()
{
    return "Output";
}

QList<QMetaType> TestDataSourcePlugin::inputTypes()
{
    //QList<QMetaType> list = QList<QMetaType>();
    ///*foreach (QVariant v, inputs)
    ///*foreach (QVariant v, inputs)
    //{
    //    list.append(v.metaType());
    //}*/

    //list.append(QMetaType::fromName("HumanoidBones"));
    return inputs;
}

QList<QMetaType> TestDataSourcePlugin::outputTypes()
{
    /*QList<QMetaType> list = QList<QMetaType>();
    foreach (QVariant v, outputs)
    {
        list.append(v.metaType());
    }
    list.append(QMetaType::fromName("HumanoidBones"));*/
    return outputs;
}
