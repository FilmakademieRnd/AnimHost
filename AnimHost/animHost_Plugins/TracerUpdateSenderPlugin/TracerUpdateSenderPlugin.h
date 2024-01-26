
#ifndef TRACERUPDATESENDERPLUGINPLUGIN_H
#define TRACERUPDATESENDERPLUGINPLUGIN_H

#include "TracerUpdateSenderPlugin_global.h"
#include "ZMQMessageHandler.h"
#include <QMetaType>
#include <QThread>
#include <QTimer>
#include <QPushButton>
#include <QValidator>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <pluginnodeinterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>
#include <zmq.hpp>
#include <nzmqt/nzmqt.hpp>

class ZMQMessageHandler;
class AnimHostMessageSender;
class TickReceiver;

//!
//! @file "TracerUpdateSenderPlugin.h"
//! @implements PluginNodeInterface
//! @brief Plugin allowing the user to send pose updates to TRACER clients
//! This plugin provides the user with a UI, with which it's possible to select an IP address
//! (and consequetly a client ID) and send poses to the listening TRACER clients. The animation
//! can be looped or not and can consist of a variable amount of poses/frames.
//! @param _animIn The animation data (consisting of one or more poses)
//! @param _characterIn The selected character to which the animation is going to be applied
//! @param _sceneNodeListIn A description of the scene of the TRACER client. Necessary to match animation data to character rig
//! @author Francesco Andreussi
//! @version 0.5
//! @date 26.01.2024
//!

class TRACERUPDATESENDERPLUGINSHARED_EXPORT TracerUpdateSenderPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginNodeInterface" FILE "TracerUpdateSenderPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    QWidget* widget;                                //!< UI container element
    QPushButton* _sendUpdateButton;                 //!< UI button element, onClick starts the animation-sending sub-thread
    QComboBox* _selectIPAddress;                    //!< UI drop down menu to select the IP address (and therefore the Client ID)
    QCheckBox* _loopCheck;                          //!< UI checkbox to enable/disable looping the animation
    QHBoxLayout* _ipAddressLayout;                  //!< UI layout element
    QRegularExpressionValidator* _ipValidator;      //!< IP Address validation regex (to be removed)

    QString _ipAddress;                             //!< The selected IP Address

    zmq::context_t* _updateSenderContext = nullptr; //!< 0MQ context to establish connection and send messages
    QThread* zeroMQSenderThread = nullptr;          //!< Sub-thread to handle message sending without making the UI thread unresponsive
    QThread* zeroMQTickReceiverThread = nullptr;    //!< Sub-thread to handle receiving synchronisation messages

    std::weak_ptr<AnimNodeData<Animation>> _animIn                          //!< @param The animation data (consisting of one or more poses)
    std::weak_ptr<AnimNodeData<CharacterObject>> _characterIn;              //!< @param The selected character to which the animation is going to be applied
    std::weak_ptr<AnimNodeData<SceneNodeObjectSequence>> _sceneNodeListIn;  //!< @param A description of the scene of the TRACER client. Necessary to match animation data to character rig
    
    QTimer* timer;                                  //!< Timer needed to keep sender and receiver in sync
    int localTime = 0;                              //! @todo move timer and local time(stamp) directly to subthread?

    AnimHostMessageSender* msgSender = nullptr;     //!< Pointer to instance of the class that builds and sends the pose updates
    TickReceiver* tickReceiver = nullptr;           //!< Pointer to instance of the class that receives sync signals

    void freeData(void* data, void* hint) {
        free(data);
    }

    int validData = -1;

public:
    TracerUpdateSenderPlugin();
    ~TracerUpdateSenderPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<TracerUpdateSenderPlugin>(new TracerUpdateSenderPlugin()); };
    
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }
    bool hasOutputRunSignal() const override { return false; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(QtNodes::PortIndex port) override;
    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;

    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    QWidget* embeddedWidget() override;

    //QTNodes
    QString category() override { return "Output"; };  // Returns a category for the node

private Q_SLOTS:
    void onChangedSelection(int index);
    void onButtonClicked();
    void onLoopCheck(int state);
    void run();
    void ticked(int externalTime);

};

#endif // TRACERUPDATESENDERPLUGINPLUGIN_H
