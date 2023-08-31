#ifndef EXAMPLEPLUGIN_H
#define EXAMPLEPLUGIN_H

#include "exampleplugin_global.h"
#include <QMetaType>
#include <plugininterface.h>


class EXAMPLEPLUGINSHARED_EXPORT ExamplePlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginInterface" FILE "exampleplugin.json")
    Q_INTERFACES(PluginInterface)

public:
    ExamplePlugin();
    ~ExamplePlugin();

    void run(QVariantList in, QVariantList& out) override;
    QObject* getObject() { return this; }



    //QTNodes
    QString name() override { return "Global Joint Positions"; };
    QString category() override;  // Returns a category for the node
    QList<QMetaType> inputTypes() override;  // Returns input data types
    QList<QMetaType> outputTypes() override;  // Returns output data types


};

#endif // EXAMPLEPLUGIN_H
