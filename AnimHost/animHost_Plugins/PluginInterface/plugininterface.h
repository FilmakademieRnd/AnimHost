#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QVariant>
#include "../../core/commondatatypes.h"
#include "plugininterface_global.h"

//!
//! \brief Interface for plugins for the AnimHost
//!
class PLUGININTERFACESHARED_EXPORT PluginInterface : public QObject
{

public:
    // main function of the plugin generating outputs based on given inputs
    virtual void run() = 0;
    // provide the name of the plugin
    virtual QString name();

    //QTNodes
    virtual QString category() = 0;  // Returns a category for the node
    virtual QList<QMetaType> inputTypes() = 0;  // Returns input data types
    virtual QList<QMetaType> outputTypes() = 0;  // Returns output data types

    //Data
    QList<QVariant> inputs; // list of input parameters
    QList<QVariant>* outputs; // list of output parameters

protected:

};

#define PluginInterface_iid "de.animhost.PluginInterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGININTERFACE_H
