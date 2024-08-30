/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program;
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */



 //!
 //! @file "AnimationSenderNode.h"
 //! @implements PluginNodeInterface
 //! @brief Plugin allowing the user to send pose updates to TRACER
 //! @param[in]  _animIn             The animation data (consisting of one or more poses)
 //! @param[in]  _characterIn        The selected character to which the animation is going to be applied
 //! @param[in]  _sceneNodeListIn    A description of the scene of the TRACER client. Necessary to match animation data to character rig
 //! @author Francesco Andreussi
 //! @version 0.5
 //! @date 26.01.2024
 //!
 /*!
  * ###Plugin class with UI elements for sending poses to the listening TRACER clients.
  * Using the UI elements, it's possible to select an IP address (and consequetly a client ID),
  * decide whether the animation is looped or not. Such animation can consist of a variable amount of poses/frames.
  */



#ifndef ANIMATIONSENDER_H
#define ANIMATIONSENDER_H

#include "../TRACERPlugin_global.h"
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

class AnimHostMessageSender;
class TickReceiver;

class TRACERPLUGINSHARED_EXPORT AnimationSenderNode : public PluginNodeInterface
{
    Q_OBJECT
private:
    // UI Elements
    QWidget* widget;                                //!< UI container element
    QVBoxLayout* _mainLayout;                        //!< UI layout element
    ;
    QHBoxLayout* _ipAddressLayout;                  //!< UI layout element
    QComboBox* _selectIPAddress;                    //!< UI drop down menu to select the IP address (and therefore the Client ID)
    QRegularExpressionValidator* _ipValidator;      //!< IP Address validation regex (to be removed)

    QCheckBox* _streamCheck;                        //!< UI checkbox element to enable/disable streaming animation

    QWidget* _streamWidget;
    QHBoxLayout* _streamLayout;//!< UI container element for controlling streaming animation (TRACER ParameterUpdate with "live" playback)
    QPushButton* _sendStreamButton;                 //!< UI button element, onClick starts the animation-sending sub-thread, toggles between "Start" and "Stop"
    QCheckBox* _loopCheck;                          //!< UI checkbox to enable/disable looping the animation

    QWidget* _enBlocWidget;
    QHBoxLayout* _enBlocLayout;//!< UI container element for controlling on bolck animation (TRACER AnimatedParameterUpdate)
    QPushButton* _sendEnBlocButton;               //!< UI button element, onClick starts the animation-sending sub-thread without iterating over whole animation sequence 


    QString _ipAddress;                             //!< The selected IP Address

    zmq::context_t* _updateSenderContext = nullptr; //!< 0MQ context to establish connection and send messages

    QThread* zeroMQSenderThread = nullptr;          //!< Sub-thread to handle message sending without making the UI thread unresponsive
    AnimHostMessageSender* msgSender = nullptr;     //!< Pointer to instance of the class that builds and sends the pose updates

    TickReceiver* tickReceiver = nullptr;           //!< Pointer to instance of the class that receives sync signals
    QThread* zeroMQTickReceiverThread = nullptr;    //!< Sub-thread to handle receiving synchronisation messages


    bool isStreaming = false;                       //!< Whether the animation is being streamed or not


    //Node Inputs
    std::weak_ptr<AnimNodeData<Animation>> _animIn;                         //!< The animation data (consisting of one or more poses) - **Data set by UI PortIn**
    std::weak_ptr<AnimNodeData<CharacterObject>> _characterIn;              //!< The selected character to which the animation is going to be applied - **Data set by UI PortIn**
    std::weak_ptr<AnimNodeData<SceneNodeObjectSequence>> _sceneNodeListIn;  //!< A description of the scene of the TRACER client. Necessary to match animation data to character rig - **Data set by UI PortIn**

    int validData = -1;

public:
    /*!
     * Instantiates message sender and tick receiver classes, moves both to their separate threads and connects the various signals to the associated functions:
     * - \c QThread::started() signal is connected to \c AnimHostMessageSender::run()
     * - \c QThread::started() signal is connected to \c TickReceiver::run()
     * - \c TickReceiver::tick() signal is connected to TracerUpdateSenderPlugin::ticked()
     */
    AnimationSenderNode();
    ~AnimationSenderNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<AnimationSenderNode>(new AnimationSenderNode()); };

    static QString Name() { return QString("AnimationSenderNode"); }

    QString category() override { return "Undefined Category"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    /*!
    * TracerUpdateSenderPlugin is a sink node, i.e. it doesn't have any output in the Qt Application's Plugin Graph.
    * Its outputs are exclusively directed to other TRACER clients
    */
    bool hasOutputRunSignal() const override { return false; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    bool  isDataAvailable() override;

    /*!
    * Connects the various signals emitted by the UI elements to slots and functions of this class:
    * - \c QComboBox::currentIndexChanged signal is connected to \c TracerUpdateSenderPlugin::onChangedSelection
    * - \c QCheckBox::stateChanged signal is connected to \c TracerUpdateSenderPlugin::onLoopCheck
    * - \c QPushButton::released signal is connected to \c TracerUpdateSenderPlugin::onButtonClicked
    * \returns A pointer to the UI elements' container
    */
    QWidget* embeddedWidget() override;

private Q_SLOTS:

    //! Slot called when the drop-down menu selection changes
    /*!
    * Replaces the IP Address with the newly selected option
    * \param index Index of the selected element in the list
    */
    void onChangedSelection(int index);

    //! Slot called when the "Send Animation" button is clicked
    /*!
    * Gets the pointers to the input ports' data, passes them to the [AnimHostMessageSender](@ref AnimHostMessageSender),
    * starts the subthread and runs the main loop of this class
    */
    void onStreamButtonClicked();

    //! Slot called when the "Send EnBloc" button is clicked
    void onEnBlocButtonClicked();

    //! Slot called when the "Loop" check gets clicked
    /*!
    * Enables/disables sending the animation as a loop (i.e. sending frames out continuously restarting the animation when at the end)
    * \param    state   Enumeration [Qt::CheckState][https://doc.qt.io/qt-6/qt.html#CheckState-enum] inherited from [QCheckBox][https://doc.qt.io/qt-6/qcheckbox.html]
    * | Int Value | Description                                                                           |
    * | --------: | :------------------------------------------------------------------------------------ |
    * | 0         | Box is unchecked                                                                      |
    * | 1         | Box is partially checked, i.e. some of the sub-boxes are checked while others aren't  |
    * | 2         | Box is checked                                                                        |
    */
    void onLoopCheck(int state);

    //! Called when plugin execution is triggered (by the Qt Application)
    /*!
    * At the moment, does nothing. Empty main loop.
    */
    void run();

    //! Slot called when the subthread responsible for receiving SYNC messages gets a message
    /*!
    * Checks whether the internal timer of the application and the other client's timer are in sync.
    * Resumes the message sending subthread if paused.
    * \param    externalTime    the time of the client, with which the application is communicating
    */
    void ticked(int externalTime);

};

#endif // ANIMATIONSENDER_H