#ifndef ANIMHOSTNODE_H
#define ANIMHOSTNODE_H

#define NODE_EDITOR_SHARED 1

#include <QtNodes/NodeDelegateModel>
#include "plugininterface.h"
#include "commondatatypes.h"
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class AnimHostNode : public NodeDelegateModel
{
    Q_OBJECT

public:
    AnimHostNode(){};
    AnimHostNode(std::shared_ptr<PluginInterface> plugin);

public:
    QString caption() const override { return QStringLiteral("AnimHostNode Caption"); }

    bool captionVisible() const override { return false; }

    QString name() const override { return(!_plugin) ?  "NONE" : _plugin->name(); }

public:
    unsigned int nPorts(PortType portType) const override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

    QWidget *embeddedWidget() override { return nullptr; }

protected:
     void compute();

protected:

    std::vector<std::weak_ptr<NodeData>> _dataIn;

    std::vector<std::shared_ptr<NodeData>> _dataOut;

private:
    std::shared_ptr<PluginInterface> _plugin;

    NodeDataType convertQMetaTypeToNodeDataType(QMetaType qType) const;
};

#endif // ANIMHOSTNODE_H
