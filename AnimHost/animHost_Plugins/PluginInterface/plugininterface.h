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

    QList<QVariant> inputs;
    QList<QVariant>* outputs;

protected:

signals:
    void done();
};

#define PluginInterface_iid "de.animhost.PluginInterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGININTERFACE_H
