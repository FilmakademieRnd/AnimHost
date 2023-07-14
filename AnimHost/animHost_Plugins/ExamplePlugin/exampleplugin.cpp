#include "exampleplugin.h"
#include <iostream>
#include <QFileInfo>
#include <QDateTime>
#include <QImage>
#include <QBuffer>

ExamplePlugin::ExamplePlugin()
{
}

// execute the main functionality of the plugin
void ExamplePlugin::run()
{
    //execute
}

QString ExamplePlugin::category()
{
    return "Generator";
}

QList<QMetaType> ExamplePlugin::inputTypes()
{
    QList<QMetaType> list = QList<QMetaType>();
    foreach (QVariant v, inputs)
    {
        list.append(v.metaType());
    }
    return list;
}

QList<QMetaType> ExamplePlugin::outputTypes()
{
    QList<QMetaType> list = QList<QMetaType>();
    foreach (QVariant v, *outputs)
    {
        list.append(v.metaType());
    }
    return list;
}
