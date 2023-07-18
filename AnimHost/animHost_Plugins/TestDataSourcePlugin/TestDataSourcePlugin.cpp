#include "TestDataSourcePlugin.h"
#include <iostream>
#include <QFileInfo>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include "../../core/commondatatypes.h"

TestDataSourcePlugin::TestDataSourcePlugin()
{
    qDebug() << "Hello Example Plugin";

 
    //Data
    //inputs.append(QMetaType::fromName("Pose"));
    outputs.append(QMetaType::fromName("Pose"));
    outputs.append(QMetaType::fromName("HumanoidBones"));
    outputs.append(QMetaType(QMetaType::Int));
    outputs.append(QMetaType(QMetaType::Float));
}

TestDataSourcePlugin::~TestDataSourcePlugin()
{
    qDebug() << "~TestDataSourcePlugin()";
}

// execute the main functionality of the plugin
void TestDataSourcePlugin::run(QVariantList in, QVariantList& out)
{
    //execute
    HumanoidBones test = in[0].value<HumanoidBones>();

    qDebug() << "Eval Example Plugin";
    qDebug() << in[0].type();

    Pose pose;

    out.append(QVariant::fromValue(pose));
}

QString TestDataSourcePlugin::category()
{
    return "Generator";
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
