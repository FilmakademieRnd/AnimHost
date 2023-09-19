
#ifndef TRACERUPDATESENDERPLUGINPLUGIN_H
#define TRACERUPDATESENDERPLUGINPLUGIN_H

#include "TracerUpdateSenderPlugin_global.h"
#include <QMetaType>
#include <QThread>
#include <QPushButton>
#include <pluginnodeinterface.h>
#include <nodedatatypes.h>
#include <zmq.hpp>
#include <nzmqt/nzmqt.hpp>
#include "AnimHostMessageSender.h"

class QPushButton;

class TRACERUPDATESENDERPLUGINSHARED_EXPORT TracerUpdateSenderPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.TracerUpdateSender" FILE "TracerUpdateSenderPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    zmq::context_t* _updateSenderContext = nullptr;
    //zmq::socket_t* _updateSenderSocket = nullptr;

    QThread* zeroMQSenderThread = nullptr;
    AnimHostMessageSender* msgSender = nullptr;

    void sendAnimData(); //possibly passing a "mode" as param

    void freeData(void* data, void* hint) {
        free(data);
    }
    QPushButton* _pushButton;

    // Input animation data (of either type animation or pose...maybe both?!)
    std::weak_ptr<AnimNodeData<Animation>> _animIn;
    std::weak_ptr<AnimNodeData<Pose>> _poseIn;

    // Output animation data to be pushed to DataHub, stored in ZeroMQ message format (conversion to native array)
    std::vector<int> _animOut;
    int validData = -1;

public:
    TracerUpdateSenderPlugin();
    ~TracerUpdateSenderPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<TracerUpdateSenderPlugin>(new TracerUpdateSenderPlugin()); };
    
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nPorts(QtNodes::PortType portType) const override;
    NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(QtNodes::PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    QWidget* embeddedWidget() override;

    //QTNodes
    QString category() override { return "Output"; };  // Returns a category for the node

private Q_SLOTS:
    //void onButtonClicked();

};

#endif // TRACERUPDATESENDERPLUGINPLUGIN_H
