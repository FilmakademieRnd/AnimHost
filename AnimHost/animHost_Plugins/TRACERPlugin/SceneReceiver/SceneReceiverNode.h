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
 //! @file "SceneReceiverNode.h"
 //! @implements PluginNodeInterface
 //! @brief Plugin allowing the user to request scene data from TRACER
 //! @param[out] characterListOut    The list of characters (CharacterObjects) to be passed to the CharacterSelectorPlugin
 //! @param[out] sceneNodeListOut    The selected character to which the animation is going to be applied
 //! @author Francesco Andreussi
 //! @version 0.5
 //! @date 26.01.2024
 //!
 /*!
  * ###Plugin class with UI elements for requesting scene information from DataHub or TRACER clients
  * Using the UI elements, it's possible to write an IP address onto which to send the request message.
  * The received replies are processed, converted in more accessible data structures, and passed along to other AnimHost plugins for further usage
  */

#ifndef SCENERECEIVER_H
#define SCENERECEIVER_H

#include "../TRACERPlugin_global.h"
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

class SceneReceiver;

class TRACERPLUGINSHARED_EXPORT SceneReceiverNode : public PluginNodeInterface
{
    Q_OBJECT
private:
    QWidget* widget;                                //!< UI container element
    QPushButton* _pushButton;                       //!< UI button element, onClick sends out the request message
    QLineEdit* _connectIPAddress;                   //!< UI single-line text box where the desired IP address is typed in
    QHBoxLayout* _ipAddressLayout;                  //!< UI layout element
    QRegularExpressionValidator* _ipValidator;      //!< IP Address validation regex
    QString _ipAddress;                             //!< The typed IP 

    //! The list of characters received from TRACER
    /*!
    * The list of characters, saved as CharacterObjectSequence. This is filled by processing the data received from TRACER - **Data sent via UI PortOut**
    */
    std::shared_ptr<AnimNodeData<CharacterObjectSequence>> characterListOut;
    //! The description of the scene of the TRACER client
    /*!
    * The description of the scene, saved as a SceneNodeSequence. This is filled by processing the data received from TRACER - **Data sent via UI PortOut**
    */
    std::shared_ptr<AnimNodeData<SceneNodeObjectSequence>> sceneNodeListOut;

    // !!!TEMPORARY!!!
    std::shared_ptr<AnimNodeData<ControlPath>> controlPathOut;

    std::shared_ptr<zmq::context_t> _sceneReceiverContext = nullptr;    //!< 0MQ context to establish connection, send and receive messages
    QThread* zeroMQSceneReceiverThread = nullptr;       //!< Sub-thread to handle sending request messages and receiving replies

    SceneReceiver* sceneReceiver;                       //!< Pointer to instance of the class that is responsible to exchange messages with the rest of the TRACER framework

public:
    //! Default constructor
    /*!
     * Instantiates the scene receiver class, moves it to its separate thread and connects the various signals to the associated functions:
     * - \c SceneReceiver::passCharacterByteArray() signal is connected to \c SceneReceiverNode::processCharacterByteData()
     * - \c SceneReceiver::passSceneNodeByteArray() signal is connected to \c SceneReceiverNode::processSceneNodeByteData()
     * - \c SceneReceiver::passHeaderByteArray() signal is connected to \c SceneReceiverNode::processHeaderByteData()
     * - \c SceneReceiverNode::requestCharacterData() signal is connected to \c SceneReceiver::requestCharacterData()
     * - \c SceneReceiverNode::requestSceneNodeData() signal is connected to \c SceneReceiver::requestSceneNodeData()
     * - \c SceneReceiverNode::requestHeaderData() signal is connected to \c SceneReceiver::requestHeaderData()
     */
    SceneReceiverNode(std::shared_ptr<zmq::context_t> zmqConext);
    ~SceneReceiverNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  nullptr; };

    static QString Name() { return QString("SceneReceiverNode"); }

    QString category() override { return "TRACER"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    bool  isDataAvailable() override;

    //! It's called when the plugin is started.
    /*!
    * It is creating the scene receiver and the sub-thread, in which it's going to be run
    */
    void run() override;

    //! Initializes the plugin's UI elements
    /*!
    * Connects the various signals emitted by the UI elements to slots and functions of this class:
    * - \c QPushButton::released signal is connected to \c TracerSceneReceiverPlugin::onButtonClicked
    * \returns A pointer to the UI elements' container
    */
    QWidget* embeddedWidget() override;

    //! Enumeration listing possible TRACER scene node types
    /*!
    * | EnumValue   | Int Value | Description                                                                                                                                   |
    * | ----------: | --------: | :-------------------------------------------------------------------------------------------------------------------------------------------- |
    * | GROUP       | 0         | Generic node                                                                                                                                  |
    * | GEO         | 1         | Node representing a geometry (more accurate description to follow)                                                                            |
    * | LIGHT       | 2         | Node representing a light (more accurate description to follow)                                                                               |
    * | CAMERA      | 3         | Node representing a virtual camera (more accurate description to follow)                                                                      |
    * | SKINNEDMESH | 4         | Node representing a skinned mesh, a subnode of a character that contains information about the skeleton (more accurate description to follow) |
    * | CHEARACTER  | 5         | Node representing a character (for now, equivalent in content to the generic node)                                                            |
    */
    enum NodeType { GROUP, GEO, LIGHT, CAMERA, SKINNEDMESH, CHARACTER };

Q_SIGNALS:
    void requestCharacterData();    //!< Triggers requesting scene character data
    void requestSceneNodeData();    //!< Triggers requesting scene description data
    void requestHeaderData();       //!< Triggers requesting scene header data
    void requestControlPathData();       //!< Triggers requesting scene header data !!!TEMPORARY!!!

private Q_SLOTS:
    //! Slot called when the "Request Data" button is clicked
    /*!
    * Trigger the request of header, scene nodes and character data.
    * This starts the feedback-loop of requesting, receiving, processing, and updating the connected application plugins
    */
    void onButtonClicked();

    //! Processing the character byte sequence received from the external TRACER client
    /*!
    * Unpacking the received byte array representing the characters (in the format of Character Packages) in the scene
    * and filling a [CharacterObjectSequence](@ref CharacterObjectSequence) with these data
    */
    void processCharacterByteData(QByteArray* charByteArray);
    //! Processing the scene node byte sequence received from the external TRACER client
    /*!
    * Unpacking the received array of bytes representing all nodes in the scene (character nodes included)
    * and filling a [SceneNodeObjectSequence](@ref SceneNodeObjectSequence) with these data
    */
    void processSceneNodeByteData(QByteArray* nodeByteArray);
    //! Processing the header byte sequence received from the external TRACER client
    /*!
    * Unpacking the received byte array representing the scene header data
    * and setting AnimHost-wide static variables in [ZMQMessageHandler](@ref ZMQMessageHandler) with these data:
    * for now only the \c targetSceneID is set
    */
    void processHeaderByteData(QByteArray* headerByteArray);

    // !!!TEMPORARY!!!
    void processControlPathByteData(QByteArray* headerByteArray);

};

#endif // SCENERECEIVER_H