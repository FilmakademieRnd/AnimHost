#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QVariant>
#include "../../core/commondatatypes.h"
#include "plugininterface_global.h"

class PLUGININTERFACESHARED_EXPORT PluginInterface : public QObject
{
    Q_OBJECT

public:
    virtual void run() = 0;
    virtual QString name() {  return metaObject()->className(); }

    //QTNodes
    virtual QString category() = 0;  // Returns a category for the node
    virtual QList<QMetaType> inputTypes() = 0;  // Returns input data types
    virtual QList<QMetaType> outputTypes() = 0;  // Returns output data types

    //Data
    QList<QVariant> inputs;
    QList<QVariant>* outputs;

protected:

signals:
    void done();
};

#define PluginInterface_iid "de.animhost.PluginInterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGININTERFACE_H
