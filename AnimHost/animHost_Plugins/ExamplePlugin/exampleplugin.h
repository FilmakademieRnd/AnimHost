#ifndef EXAMPLEPLUGIN_H
#define EXAMPLEPLUGIN_H

#include "exampleplugin_global.h"
#include <plugininterface.h>


class EXAMPLEPLUGINSHARED_EXPORT ExamplePlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginInterface" FILE "exampleplugin.json")
    Q_INTERFACES(PluginInterface)

public:
    ExamplePlugin();
    void run() override;
    QObject* getObject() { return this; }

    //QTNodes
    QString category() override;  // Returns a category for the node
    QList<QMetaType> inputTypes() override;  // Returns input data types
    QList<QMetaType> outputTypes() override;  // Returns output data types

    //Data
    QList<QVariant> inputs = { QVariant(10),        //int
                               QVariant(5.f) };     //float
    QList<QVariant>* outputs = new QList<QVariant>{ QVariant(10),        //int
                                                    QVariant(5.f) };     //float

};

#endif // EXAMPLEPLUGIN_H
