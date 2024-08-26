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
//#include <nzmqt/nzmqt.hpp>
#include <zmq.hpp>


class TRACERUPDATESENDERPLUGINSHARED_EXPORT AnimHostMessageSender : public ZMQMessageHandler {
    
    Q_OBJECT

    public:

    enum SendMode {
		STREAMSTART,
        STREAMSTOP,
		ENBLOCK
	};

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
        qDebug() << "~AnimHostMessageSender()";
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

    //! Resumes broadcasting poses
    /*!
     * Sets \c _paused to false and wakes the thread that is waiting on the reconnectWaitCondition
     */
    void resumeSendFrames();

    



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

    //! Creating a TRACER Parameter Update Message Body, given a specific vector of floats to send
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
    template<typename T>
    QByteArray CreateParameterUpdateBody(byte sceneID, uint16_t objectID, uint16_t parameterID, ZMQMessageHandler::ParameterType parameterType, T payload);

    template<>
    QByteArray CreateParameterUpdateBody<std::string>(byte sceneID, uint16_t objectID, uint16_t parameterID, ZMQMessageHandler::ParameterType parameterType, std::string payload);

    template <typename T>
    QByteArray createAnimationParameterUpdateBody(byte sceneID, int objectID, int parameterID, ZMQMessageHandler::ParameterType parameterType,
        ZMQMessageHandler::AnimationKeyType keytype, const std::vector < std::pair<float, T>>& Keys, const std::vector < std::pair<float, T>>& tangentKeys, int frame);
    
    //! Templated function to convert a single value to a series of bytes
    template<typename T>
    void serializeValue(QDataStream& stream, const T& value) {
        stream.writeRawData(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    //! Specialization for glm::quat
    //! TRACER expects quaternions to be sent as 4 floats with the order x,y,z,w
    //! glm::quat is stored as w,x,y,z
    template<>
    void serializeValue<glm::quat>(QDataStream& stream, const glm::quat& value) {
        stream << value.x << value.y << value.z << value.w; 
    }

    //! Specialization for std::string
    template<>
    void serializeValue<std::string>(QDataStream& stream, const std::string& value) {
		stream.writeRawData(value.c_str(), value.size());
	}

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

    void SerializeAnimation(std::shared_ptr<Animation> animData, std::shared_ptr<CharacterObject> character, 
                                std::shared_ptr<SceneNodeObjectSequence> sceneNodeList, QByteArray* byteArray, int frame);

    //! Indicates whether the sent animation will loop or not
    /*!
    * Is modified through the Qt Application UI.
    * Is ignored if the Animation that has to be sent contains only one pose/frame.
    */
    bool loop = false;

    bool streamAnimation = false; //!< Indicates whether the animation data has to be streamed frame by frame or send en bloc

    bool sendBlock = false; //!< Indicates whether the animation data has to be sent en bloc

    int animDataSize = 0; //!< The size of the animation data to be sent

    //! Set sending mode to stream animation frame by frame or send en bloc
    //! 
    void setStreamAnimation(SendMode sendMode) {
        
        mutex.lock();

        switch(sendMode) {
			case SendMode::STREAMSTART:
				streamAnimation = true;
				sendBlock = false;
				break;
			case SendMode::STREAMSTOP:
				streamAnimation = false;
				sendBlock = false;
				break;
			case SendMode::ENBLOCK:
				streamAnimation = false;
				sendBlock = true;
				break;
		}
    
        mutex.unlock();
        

      
    }


    //! The delta, by which the timestamp is increased for sending the next frame
    /*!
    * The default is 1, it's higher if the sent animation has a **lower** frame rate w.r.t. the target application
    */
    float deltaTimeStamp = 1;

    //! The delta, by which the frame count is increased for selecting the next animation frame to be sent
    /*!
    * The default is 1, it's higher if the sent animation has a **higher** frame rate w.r.t. the target application
    */
    float deltaAnimFrame = 1;

    //! The buffer slot, in which the first pose of the animation has to be written
    /*!
    * \todo To be received via handshake before starting pose streaming?
    */
    int initialOffset = 0;




    std::shared_ptr<Animation> animData = nullptr;                      //!< Animation data to be serialised and sent
    std::shared_ptr<CharacterObject> charObj = nullptr;                 //!< Character Object to which the animation will be applied
    std::shared_ptr<SceneNodeObjectSequence> sceneNodeList = nullptr;   //!< Description of the scene to be updated (contains the character that will be animated)

    private:
    //! ZeroMQ Socket used to send animation data
    zmq::socket_t* sendSocket = nullptr;

    //syncMessage
    //byte syncMessage[3] = { targetHostID,0,MessageType::EMPTY };


    private:

    //! Function to initialise and continouesly streaming of animation data frame by frame
    void streamAnimationData();

    //! Function to initialise and stream a animation data as a AnimationParameterUpdateMessage en bloc
    void sendAnimationDataBlock();


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
