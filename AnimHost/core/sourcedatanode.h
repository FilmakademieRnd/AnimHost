#ifndef SOURCEDATANODE_H
#define SOURCEDATANODE_H

#define NODE_EDITOR_SHARED 1

#include <QtNodes/NodeDelegateModel>
#include "plugininterface.h"
#include "commondatatypes.h"
#include "nodedatatypes.h"
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class ANIMHOSTCORESHARED_EXPORT SourceDataNode : public NodeDelegateModel
{
    Q_OBJECT

public:
    SourceDataNode();


public:
    QString caption() const override { return QStringLiteral("Test Data Source"); }

    bool captionVisible() const override { return false; }

    QString name() const override { return "Test Data Source"; }

public:
    unsigned int nPorts(PortType portType) const override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override {};

    QWidget *embeddedWidget() override { return nullptr; }


protected:
    std::vector<std::shared_ptr<NodeData>> _dataOut;

};

#endif // ANIMHOSTNODE_H
