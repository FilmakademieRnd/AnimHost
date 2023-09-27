
#ifndef DATAEXPORTPLUGINPLUGIN_H
#define DATAEXPORTPLUGINPLUGIN_H

#include "DataExportPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>

class QPushButton;

class DATAEXPORTPLUGINSHARED_EXPORT DataExportPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.DataExport" FILE "DataExportPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    QPushButton* _pushButton;

public:
    DataExportPlugin();
    ~DataExportPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<DataExportPlugin>(new DataExportPlugin()); };

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

#endif // DATAEXPORTPLUGINPLUGIN_H
