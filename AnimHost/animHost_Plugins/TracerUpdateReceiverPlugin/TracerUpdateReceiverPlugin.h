

//!
//! @file "TracerUpdateReceiverPlugin.h"
//! @implements PluginNodeInterface
//! @brief Plugin listening to Parameter Updates
//! @param[in]      _parameterUpdateMessage The message emitted from a TRACER application as a result of a parameter change in a TRACER scene
//! @param[in]      _characterListIn        List of Charaters previously received by AnimHost
//! @param[out]     _controlPathOut  The new ControlPath resulting from the updating of the ControlPoints from a TRACER applications via a ParameterUpdateMessage
//! @author Francesco Andreussi
//! @version 0.5
//! @date 05.03.2024
//!
/*!
 * ###Plugin class that listens to Parameter Updates that are directed towards any Waypoint parameters of a Control Path of a Character.
 * The Plugin filters out messages that do not contain the SceneObjectID of one of the Characters previously received by AnimHost
 * (\see @class TracerSceneReceiverPlugin); it also ignores updates not directed at the ControlPath Parameter of such CharacterObjects.
 */

#ifndef TRACERUPDATERECEIVERPLUGINPLUGIN_H
#define TRACERUPDATERECEIVERPLUGINPLUGIN_H

#include "TracerUpdateReceiverPlugin_global.h"
#include "ZMQMessageHandler.h"
#include <QMetaType>
#include <pluginnodeinterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>
#include <zmq.hpp>
#include <nzmqt/nzmqt.hpp>

class TracerUpdateReceiver;

class TRACERUPDATERECEIVERPLUGINSHARED_EXPORT TracerUpdateReceiverPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginNodeInterface" FILE "TracerUpdateReceiverPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    zmq::context_t* _updateReceiverContext = nullptr;       //!< 0MQ context to establish connection and receive messages
    QThread* zeroMQUpdateReceiverThread = nullptr;          //!< Sub-thread to handle message listening to and receiving messages without making the UI thread unresponsive

    std::weak_ptr<AnimNodeData<CharacterObjectSequence>> _characterListIn;      //!< A list of characters present in the previously received TRACER. Necessary to match animation data to character rig - **Data set by UI PortIn**
    std::shared_ptr<AnimNodeData<ControlPath>> _controlPathOut;                 //!< The control path that the character has to follow. It will be fed to a NN model for generating a character animation - **Data sent via UI PortOut**

    TracerUpdateReceiver* msgReceiver = nullptr;           //!< Pointer to instance of the class that receives update messages

public:
    TracerUpdateReceiverPlugin();
    ~TracerUpdateReceiverPlugin();
    //! Initialising function returning pointer to instance of self (calls the default constructor)
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<TracerUpdateReceiverPlugin>(new TracerUpdateReceiverPlugin()); };

    //! Returning Plugin name, called by Qt Application
    QString caption() const override { return this->name(); }

    //! Whether name is visible on UI, called by Qt Application
    bool captionVisible() const override { return true; }

    //! Whether Plugin passes on the run signal, called by Qt Application
    /*!
    * TracerUpdateSenderPlugin is a sink node, i.e. it doesn't have any output in the Qt Application's Plugin Graph.
    * Its outputs are exclusively directed to other TRACER clients
    */
    bool hasOutputRunSignal() const override { return true; }

    //! Public function called by Qt Application returning number of in and out ports
    /*!
    * \param  portType (enum - 0: IN, 1: OUT, 2: NONE)
    * \return number of IN and OUT ports
    */
    unsigned int nDataPorts(QtNodes::PortType portType) const override;

    //! Public function called by Qt Application returning which datatype is associated to a specific port
    /*!
    * \param  portType (enum - 0: IN, 1: OUT, 2: NONE)
    * \param  portIndex (unsinged int with additional checks)
    * \return datatype associated to the portIndex-th IN or OUT port
    */
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(QtNodes::PortIndex port) override;            //!< Given a port index, returns the type of data of the corresponding OUT port
    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;     //!< Given a port index, processes and returns a pointer to the data of the corresponding OUT port
    bool isDataAvailable() override;                                                //!< Checks whether the input data of the plugin is valid and available. If not the plugin run function is not going to be run

    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override; //!< Given a port index, processes the data of the corresponding IN port

    //! Initializes the plugin's UI elements: empty for now
    QWidget* embeddedWidget() override { return nullptr; };

    //! Defines the category of this plugin as **Input** for the plugin selection menu of the AnimHost Application, called by Qt Application
    QString category() override { return "Input"; };

private Q_SLOTS:

    //! Called when plugin execution is triggered (by the Qt Application)
        /*!
        * At the moment, does nothing. Empty main loop.
        */
    void run();
    
    //! Slot called when the subthread responsible for receiving the Update Messages receives something
    /*!
    * Filters the update messages, keeping only the ones that are referred to the previously received characters and, of all of those,
    * it discards the ones not related to the ControlPath parameter.
    * \param    paramUpdateMsg    received Parameter Update Message
    */
    void processParameterUpdate(QByteArray* paramUpdateMsg);

};

#endif // TRACERUPDATERECEIVERPLUGINPLUGIN_H
