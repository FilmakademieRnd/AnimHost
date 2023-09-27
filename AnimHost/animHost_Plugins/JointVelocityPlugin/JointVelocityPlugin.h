
#ifndef JOINTVELOCITYPLUGINPLUGIN_H
#define JOINTVELOCITYPLUGINPLUGIN_H

#include "JointVelocityPlugin_global.h"
#include <QMetaType>
#include <plugininterface.h>

class JOINTVELOCITYPLUGINSHARED_EXPORT JointVelocityPlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.JointVelocity" FILE "JointVelocityPlugin.json")
    Q_INTERFACES(PluginInterface)

public:
    JointVelocityPlugin();
    ~JointVelocityPlugin();
    void run(QVariantList in, QVariantList& out) override;
    QObject* getObject() { return this; }

    //QTNodes
    QString category() override;  // Returns a category for the node
    QList<QMetaType> inputTypes() override;  // Returns input data types
    QList<QMetaType> outputTypes() override;  // Returns output data types


};

#endif // JOINTVELOCITYPLUGINPLUGIN_H
