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
    inputs.append(QMetaType::fromName("Skeleton"));
    inputs.append(QMetaType::fromName("Animation"));

    outputs.append(QMetaType::fromName("Pose"));
}

ExamplePlugin::~ExamplePlugin()
{
    qDebug() << "Good Bye Example Plugin";
}

// execute the main functionality of the plugin
void ExamplePlugin::run(QVariantList in, QVariantList& out)
{
    qDebug() << Q_FUNC_INFO;
    //execute
    Skeleton skeleton = in[0].value<Skeleton>();
    Animation animation = in[1].value<Animation>();

    qDebug() << in[0].userType();
    qDebug() << QMetaType::fromName("Skeleton").id();

    qDebug() << in[1].userType();
    qDebug() << QMetaType::fromName("Animation").id();

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
