#ifndef PLUGINNODEINTERFACE_H
#define PLUGINNODEINTERFACE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QVariant>
#include "plugininterface.h"
#include <QtNodes/NodeDelegateModel>
#include <nodedatatypes.h>
#include "pluginnodeinterface_global.h"

//!
//! \brief Interface for plugins for the AnimHost
//!
class PLUGINNODEINTERFACESHARED_EXPORT PluginNodeInterface : public QtNodes::NodeDelegateModel
{

private:
    std::shared_ptr<AnimNodeData<RunSignal>> _runSignal = nullptr;

public:

    PluginNodeInterface() {};
    PluginNodeInterface(const PluginNodeInterface& p) {};

    virtual std::unique_ptr<NodeDelegateModel>  Init() { throw; };

    QString name() const override { return metaObject()->className(); };

    virtual QString category() { throw; }; 

    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nPorts(QtNodes::PortType portType) const override;

    /**
     * Return the number of data ports in- and output of the node. 
     * By default +1 added for run signal port.
     * 
     * \param portType Selector of in- or output type
     * \return 
     */
    virtual unsigned int nDataPorts(QtNodes::PortType portType) const = 0;

    virtual bool hasInputRunSignal() const { return true; }
    virtual bool hasOutputRunSignal() const { return true; }


    NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    
    /*! Return the data type of data in - or output on given port.
      @param[in] portIndex = 0 is reserved for run signal and handeled before dataPortType is called.*/
    virtual NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const = 0;

    std::shared_ptr<NodeData> outData(QtNodes::PortIndex port) override;
    virtual std::shared_ptr<NodeData>  processOutData(QtNodes::PortIndex port) = 0;

     
    void setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex);
    virtual void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) = 0;

    void emitDataUpdate(QtNodes::PortIndex portIndex);

    void emitRunNextNode();

    void emitDataInvalidated(QtNodes::PortIndex portIndex);

    virtual void run() = 0;

    QWidget* embeddedWidget() override { throw; };




};

#define PluginNodeInterface_iid "de.animhost.PluginNodeInterface"

Q_DECLARE_INTERFACE(PluginNodeInterface, PluginNodeInterface_iid)

#endif // PLUGINNODEINTERFACE_H
