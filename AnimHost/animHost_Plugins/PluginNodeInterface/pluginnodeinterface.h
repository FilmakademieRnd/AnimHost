#ifndef PLUGINNODEINTERFACE_H
#define PLUGINNODEINTERFACE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QVariant>
#include "plugininterface.h"
#include <QtNodes/NodeDelegateModel>
#include "pluginnodeinterface_global.h"

//!
//! \brief Interface for plugins for the AnimHost
//!
class PLUGINNODEINTERFACESHARED_EXPORT PluginNodeInterface : public QtNodes::NodeDelegateModel
{

public:

    PluginNodeInterface() {};
    PluginNodeInterface(const PluginNodeInterface& p) {};

    virtual std::unique_ptr<NodeDelegateModel>  Init() { throw; };

    QString name() const override { return metaObject()->className(); };

    virtual QString category() { throw; }; 

    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nPorts(QtNodes::PortType portType) const override { throw; };
    NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override { throw; };

    std::shared_ptr<NodeData> outData(QtNodes::PortIndex port) override { throw; };
    void setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override {};

    QWidget* embeddedWidget() override { throw; };


};

#define PluginNodeInterface_iid "de.animhost.PluginNodeInterface"

Q_DECLARE_INTERFACE(PluginNodeInterface, PluginNodeInterface_iid)

#endif // PLUGINNODEINTERFACE_H
