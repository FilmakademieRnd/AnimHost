
#ifndef BASICONNXPLUGINPLUGIN_H
#define BASICONNXPLUGINPLUGIN_H

#include "BasicOnnxPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>

class QPushButton;

class BASICONNXPLUGINSHARED_EXPORT BasicOnnxPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.BasicOnnx" FILE "BasicOnnxPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    QPushButton* _pushButton;

public:
    BasicOnnxPlugin();
    ~BasicOnnxPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<BasicOnnxPlugin>(new BasicOnnxPlugin()); };

    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nPorts(QtNodes::PortType portType) const override;
    NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(QtNodes::PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    QWidget* embeddedWidget() override;

    //QTNodes
    QString category() override { return "Undefined Category"; };  // Returns a category for the node

private Q_SLOTS:
    void onButtonClicked();

};

#endif // BASICONNXPLUGINPLUGIN_H
