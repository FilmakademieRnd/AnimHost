
#ifndef TRACERSCENERECEIVERPLUGINPLUGIN_H
#define TRACERSCENERECEIVERPLUGINPLUGIN_H

#include "TracerSceneReceiverPlugin_global.h"
#include "ZMQMessageHandler.h"


#include <QMetaType>
#include <QThread>
#include <QTimer>
#include <QValidator>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>

#include <pluginnodeinterface.h>
#include <nodedatatypes.h>
#include <commondatatypes.h>

#include <zmq.hpp>
#include <nzmqt/nzmqt.hpp>

class SceneReceiver;

class TRACERSCENERECEIVERPLUGINSHARED_EXPORT TracerSceneReceiverPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.TracerSceneReceiver" FILE "TracerSceneReceiverPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    QWidget* widget;
    QPushButton* _pushButton;
    QLineEdit* _connectIPAddress;
    QHBoxLayout* _ipAddressLayout;
    QRegularExpressionValidator* _ipValidator;
    QString _ipAddress;

    std::shared_ptr<AnimNodeData<CharacterObjectSequence>> characterListOut;

    zmq::context_t* _sceneReceiverContext = nullptr;
    QThread* zeroMQSceneReceiverThread = nullptr;

    SceneReceiver* sceneReceiver;

public:
    TracerSceneReceiverPlugin();
    ~TracerSceneReceiverPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<TracerSceneReceiverPlugin>(new TracerSceneReceiverPlugin()); };

    QString category() override { return "Import"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    void run() override;
    
    QWidget* embeddedWidget() override;

    enum NodeType { GROUP, GEO, LIGHT, CAMERA, SKINNEDMESH };

Q_SIGNALS:
    void requestCharacterData();
    void requestSceneNodeData();
    void requestHeaderData();

private Q_SLOTS:
    void onButtonClicked();
    void processCharacterByteData(QByteArray* charByteArray);
    void processSceneNodeByteData(QByteArray* nodeByteArray);
    //void processHeaderByteData(QByteArray* headerByteArray);

};

#endif // TRACERSCENERECEIVERPLUGINPLUGIN_H
