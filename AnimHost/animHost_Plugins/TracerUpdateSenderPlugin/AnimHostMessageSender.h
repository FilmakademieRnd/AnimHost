
//!
//! \file "AnimhostMessageSender.h"
//! \implements ZMQMessageHandler
//! \brief Class used to build and send udpate messages to TRACER clients     
//! \author Francesco Andreussi
//! \version 0.5
//! \date 26.01.2024
//!
/*!
 * ###This class is instanced by the [TracerUpdateSenderPlugin](@ref TracerUpdateSenderPlugin) and is run in a subthread.
 * The class handles the connection to DataHub and the other TRACER clients, it builds a TRACER Update Message given a certain payload,
 * like bool, int, float, vector of floats (vec2, vec3, vec4, quat, colour), or string
 */

#ifndef ANIMHOSTMESSAGESENDER_H
#define ANIMHOSTMESSAGESENDER_H

#include "ZMQMessageHandler.h"
#include "TracerUpdateSenderPlugin.h"

#include <QMutex>
#include <QMultiMap>
#include <QElapsedTimer>
#include <nzmqt/nzmqt.hpp>
#include <zmq.h>


class TRACERUPDATESENDERPLUGINSHARED_EXPORT AnimHostMessageSender : public ZMQMessageHandler {
    
    Q_OBJECT

    public:
    //! Default constructor
    AnimHostMessageSender() {}
    //! \brief Constructor
    //! \param[in]  m_debugState    boolean inherited from [ZMQMessageHandler](@ref ZMQMessageHandler) indicating whether to print debug messages
    //! \param[in]  m_context       pointer to ZMQ Context instance
    AnimHostMessageSender(bool m_debugState, zmq::context_t * m_context) {
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;
    }
    //! Default destructor
    ~AnimHostMessageSender() {
        //sendSocket->close();
    }

    //! Request this process to start working
    /*!
    * Sets \c _working to true, \c _stopped and \c _paused to false and creates a new publishing \c sendSocket for message broadcasting
    */
    void requestStart() override;

    //! Request this process to stop working
    /*!
    * Sets \c _stopped to true and \c _paused to false. -Should- close \c sendSocket and cleanup
    */
    void requestStop() override;

    //! Setting data necessary to create new TRACER Update Message representing a character pose
    /*!
    * The character data is needed to get the parameter-to-bone mapping (which parameter modifies which bone). The bone is represented as the index of a node
    * in the scene description. The node has a name, which can be matched to the name of a bone in the animation data, and from this it is possible to extract
    * the pose of the bone to be sent as a TRACER Parameter Update Message
    * @param[in]    ad  The animation data: collection of Bones, which contains a sequence of poses of a specific skeletal bone
    * @param[in]    co  The character data: collection of all the data related to a character in a TRACER Application (e.g. Unity/Unreal/Blender VPET)
    * @param[in]    snl The scene description: collection of nodes in the Scene listed sequentially (not hierarchically)
    */
    void setAnimationAndSceneData(std::shared_ptr<Animation> ad, std::shared_ptr<CharacterObject> co, std::shared_ptr<SceneNodeObjectSequence> snl);

    //! Creating a TRACER Update Message Body, given a specific bool to send
    /*! 
    * Creating a TRACER Update Message Body, given a specific parameter to send. When a multiple Object Parameters have to be updated the various message bodies can be 
    * concatenated into a larger message, which will be unpacked and processed by the receiver
    * @param[in]    sceneID     1-byte value representing the ID of the TRACER Scene to be addressed
    * @param[in]    objectID    2-byte value representing the ID of the specific Scene Object to be addressed
    * @param[in]    parameterID 2-byte value representing the ID of the specific Object Parameter to be addressed
    * @param[in]    paramType   The type of the parameter (expressed in "TRACER terms") [ZMQMessageHandler::ParameterType](@ref ZMQMessageHandler::ParameterType)
    * @param[in]    payload     The actual values to be included in the Update Message
    * @sa ZMQMessageHandler::ParameterType
    */
    QByteArray createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType paramType,
                                 bool payload);
    //! Creating a TRACER Update Message Body, given a specific integer to send
    /*!
    * Creating a TRACER Update Message Body, given a specific parameter to send. When a multiple Object Parameters have to be updated the various message bodies can be
    * concatenated into a larger message, which will be unpacked and processed by the receiver
    * @param[in]    sceneID     1-byte value representing the ID of the TRACER Scene to be addressed
    * @param[in]    objectID    2-byte value representing the ID of the specific Scene Object to be addressed
    * @param[in]    parameterID 2-byte value representing the ID of the specific Object Parameter to be addressed
    * @param[in]    paramType   The type of the parameter (expressed in "TRACER terms") [ZMQMessageHandler::ParameterType](@ref ZMQMessageHandler::ParameterType)
    * @param[in]    payload     The actual values to be included in the Update Message
    * @sa ZMQMessageHandler::ParameterType
    */
    QByteArray createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType paramType,
                                 std::int32_t payload);
    //! Creating a TRACER Update Message Body, given a specific float to send
    /*!
    * Creating a TRACER Update Message Body, given a specific parameter to send. When a multiple Object Parameters have to be updated the various message bodies can be
    * concatenated into a larger message, which will be unpacked and processed by the receiver
    * @param[in]    sceneID     1-byte value representing the ID of the TRACER Scene to be addressed
    * @param[in]    objectID    2-byte value representing the ID of the specific Scene Object to be addressed
    * @param[in]    parameterID 2-byte value representing the ID of the specific Object Parameter to be addressed
    * @param[in]    paramType   The type of the parameter (expressed in "TRACER terms") [ZMQMessageHandler::ParameterType](@ref ZMQMessageHandler::ParameterType)
    * @param[in]    payload     The actual values to be included in the Update Message
    * @sa ZMQMessageHandler::ParameterType
    */
    QByteArray createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType paramType,
                                 float payload);
    //! Creating a TRACER Update Message Body, given a specific string to send
    /*!
    * Creating a TRACER Update Message Body, given a specific parameter to send. When a multiple Object Parameters have to be updated the various message bodies can be
    * concatenated into a larger message, which will be unpacked and processed by the receiver
    * @param[in]    sceneID     1-byte value representing the ID of the TRACER Scene to be addressed
    * @param[in]    objectID    2-byte value representing the ID of the specific Scene Object to be addressed
    * @param[in]    parameterID 2-byte value representing the ID of the specific Object Parameter to be addressed
    * @param[in]    paramType   The type of the parameter (expressed in "TRACER terms") [ZMQMessageHandler::ParameterType](@ref ZMQMessageHandler::ParameterType)
    * @param[in]    payload     The actual values to be included in the Update Message
    * @sa ZMQMessageHandler::ParameterType
    */
    QByteArray createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType paramType,
                                 std::string payload);
    //! Creating a TRACER Update Message Body, given a specific vector of floats to send
    /*!
    * Creating a TRACER Update Message Body, given a specific parameter to send. When a multiple Object Parameters have to be updated the various message bodies can be
    * concatenated into a larger message, which will be unpacked and processed by the receiver
    * @param[in]    sceneID     1-byte value representing the ID of the TRACER Scene to be addressed
    * @param[in]    objectID    2-byte value representing the ID of the specific Scene Object to be addressed
    * @param[in]    parameterID 2-byte value representing the ID of the specific Object Parameter to be addressed
    * @param[in]    paramType   The type of the parameter (expressed in "TRACER terms") [ZMQMessageHandler::ParameterType](@ref ZMQMessageHandler::ParameterType)
    * @param[in]    payload     The actual values to be included in the Update Message
    * @sa ZMQMessageHandler::ParameterType
    */
    QByteArray createMessageBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType paramType,
                                 std::vector<float> payload);
    
    //! Converting any vector to a series of bytes added to a larger series of bytes
    /*!
    * Function converting any vector representable in TRACER into a series of bytes added to a larger series of bytes.
    * It is called by the [createMessageBody](@ref createMessageBody) function
    * @param[out]   dest    Pointer to byte series, to which the serialised vector will be added
    * @param[in]    _vector A vector of floats to be serialised
    * @param[in]    type    The type of the parameter in "TRACER terms" [ZMQMessageHandler::ParameterType](@ref ZMQMessageHandler::ParameterType) necessary to serialise the values correctly
    */
    void SerializeVector(byte* dest, std::vector<float> _vector, ZMQMessageHandler::ParameterType type);

    //! Converting an animation frame to a series of bytes
    /*!
    * Converting an animation frame represented by a list of quaternions [Animation](@ref Animation) into a series of bytes.
    * It is called by the main loop of the class \ref run. It calls \ref createMessageBody.
    * @param[in]    animData        The Animation data as a collection of bone pose sequences
    * @param[in]    character       The Character data, coinsisting of names, IDs, and all the mappings related to the character's subcomponents
    * @param[in]    sceneNodeList   A list of scene nodes that represent the TRACER Scene to address
    * @param[out]   byteArray       The array of bytes to be modified by appending to the end the newly generated Message Body
    * @param[in]    frame           The frame of the animation to be serialised. If not set, the first frame will be used
    */
    void SerializePose(std::shared_ptr<Animation> animData, std::shared_ptr<CharacterObject> character,
                       std::shared_ptr<SceneNodeObjectSequence> sceneNodeList, QByteArray* byteArray, int frame = 0);

    //! Indicates whether the sent animation will loop or not
    /*!
    * Is modified through the Qt Application UI.
    * Is ignored if the Animation that has to be sent contains only one pose/frame.
    */
    bool loop = false;

    std::shared_ptr<Animation> animData = nullptr;                      //!< Animation data to be serialised and sent
    std::shared_ptr<CharacterObject> charObj = nullptr;                 //!< Character Object to which the animation will be applied
    std::shared_ptr<SceneNodeObjectSequence> sceneNodeList = nullptr;   //!< Description of the scene to be updated (contains the character that will be animated)

    private:
    //! ZeroMQ Socket used to send animation data
    zmq::socket_t* sendSocket = nullptr;

    //syncMessage
    //byte syncMessage[3] = { targetHostID,0,MessageType::EMPTY };

    signals:
    //signal emitted when process requests to work
    //void startRequested();

    //! Signal emitted when process is finished
    void stopped();

    public Q_SLOTS:
    //! Main loop, executes all operations
    /*!
     * Gets triggered by the Qt Application UI thread, when the "Send animation" button of the TracerUpdateSender Plugin is clicked.
     * The first time it is called, it opens a socket. Then, for every frame present in the animation, it creates a message given the
     * data coming from the other plugins (animation, character and scene data), and sends the message over the 0MQ connection.
     */
    void run();
};

#endif // ANIMHOSTMESSAGESENDER_H
