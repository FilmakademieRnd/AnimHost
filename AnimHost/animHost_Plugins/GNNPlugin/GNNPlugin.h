
#ifndef GNNPLUGINPLUGIN_H
#define GNNPLUGINPLUGIN_H

#include "GNNPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>
#include "GNNController.h"

class QPushButton;

class GNNPLUGINSHARED_EXPORT GNNPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.GNN" FILE "GNNPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    QPushButton* _pushButton;
    std::unique_ptr<GNNController> controller;



    std::weak_ptr<AnimNodeData<Animation>> _animationIn;
    std::weak_ptr<AnimNodeData<Skeleton>> _skeletonIn;

public:
    GNNPlugin();
    ~GNNPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<GNNPlugin>(new GNNPlugin()); };

    QString category() override { return "Undefined Category"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    void run() override;

    QWidget* embeddedWidget() override;

private Q_SLOTS:
    void onButtonClicked();

};

#endif // GNNPLUGINPLUGIN_H
