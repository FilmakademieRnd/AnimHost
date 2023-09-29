#ifndef JOINTPOSITIONPLUGIN_H
#define JOINTPOSITIONPLUGIN_H

#include "JointPositionPlugin_global.h"
#include <QMetaType>
#include <plugininterface.h>


class JOINTPOSITIONPLUGINSHARED_EXPORT JointPositionPlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginInterface" FILE "JointPositionPlugin.json")
    Q_INTERFACES(PluginInterface)

public:
    JointPositionPlugin();
    ~JointPositionPlugin();

    void run(QVariantList in, QVariantList& out) override;
    QObject* getObject() { return this; }



    //QTNodes
    QString name() override { return "Global Joint Positions"; };
    QString category() override;  // Returns a category for the node
    QList<QMetaType> inputTypes() override;  // Returns input data types
    QList<QMetaType> outputTypes() override;  // Returns output data types


};

#endif // JOINTPOSITIONPLUGIN_H
