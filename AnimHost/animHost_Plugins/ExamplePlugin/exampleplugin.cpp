#include "exampleplugin.h"
#include <iostream>
#include <QFileInfo>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include "../../core/commondatatypes.h"

ExamplePlugin::ExamplePlugin()
{
    qDebug() << "Hello Example Plugin";

 
    //Data
    inputs.append(QMetaType::fromName("HumanoidBones"));
    outputs.append(QMetaType::fromName("Pose"));
}

ExamplePlugin::~ExamplePlugin()
{
    qDebug() << "Good Bye Example Plugin";
}

// execute the main functionality of the plugin
void ExamplePlugin::run(QVariantList in, QVariantList& out)
{
    //execute
    HumanoidBones test = in[0].value<HumanoidBones>();

    qDebug() << "Eval Example Plugin";
    qDebug() << in[0].userType();

    qDebug() << QMetaType::fromName("HumanoidBones").id();

    Pose pose;

    out.append(QVariant::fromValue(pose));
}

QString ExamplePlugin::category()
{
    return "Generator";
}

QList<QMetaType> ExamplePlugin::inputTypes()
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

QList<QMetaType> ExamplePlugin::outputTypes()
{
    /*QList<QMetaType> list = QList<QMetaType>();
    foreach (QVariant v, outputs)
    {
        list.append(v.metaType());
    }
    list.append(QMetaType::fromName("HumanoidBones"));*/
    return outputs;
}
