#ifndef EXAMPLEPLUGIN_H
#define EXAMPLEPLUGIN_H

#include "TestDataSourcePlugin_global.h"
#include <QMetaType>
#include <plugininterface.h>


class TESTDATASOURCEPLUGINSHARED_EXPORT TestDataSourcePlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginInterface" FILE "TestDataSourcePlugin.json")
    Q_INTERFACES(PluginInterface)

public:
    TestDataSourcePlugin();
    ~TestDataSourcePlugin();
    void run(QVariantList in, QVariantList& out) override;
    QObject* getObject() { return this; }

    //QTNodes
    QString name() override { return "CSV Export"; }
    QString category() override;  // Returns a category for the node
    QList<QMetaType> inputTypes() override;  // Returns input data types
    QList<QMetaType> outputTypes() override;  // Returns output data types


};

#endif // EXAMPLEPLUGIN_H
