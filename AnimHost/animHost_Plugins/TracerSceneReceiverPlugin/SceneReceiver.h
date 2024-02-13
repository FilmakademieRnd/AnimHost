
//!
//! \file "SceneReceiver.h"
//! \implements ZMQMessageHandler
//! \brief Class used to receive Scene and Character data from the the scene to update
//! \author Francesco Andreussi
//! \version 0.5
//! \date 26.01.2024
//!
/*!
 * ###This class is instanced by the [TracerSceneReceiverPlugin](@ref TracerSceneReceiverPlugin) and is run in a subthread.
 * The class requests all the relevant data from the TRACER client, to which the AnimHost Application will send the Update Messages
 */

#ifndef SCENERECEIVER_H
#define SCENERECEIVER_H

#include "ZMQMessageHandler.h"
#include "TracerSceneReceiverPlugin.h"

#include <QMutex>
#include <QThread>
#include <nzmqt/nzmqt.hpp>

class TRACERSCENERECEIVERPLUGINSHARED_EXPORT SceneReceiver : public ZMQMessageHandler {

    Q_OBJECT

    public:
    //! Default constructor
    SceneReceiver() {}

    //! Constructor
    /*!
    * \param[in]    m_TSRPlugin     The TracerSceneReceiverPlugin instance that calls the constructor
    * \param[in]    _senderIP       IP address for the socket connection
    * \param[in]    m_debugState    Whether the class should print out debug messages
    * \param[in]    m_context       A pointer to the 0MQ Context instanciated by \c m_TSRPlugin
    */
    SceneReceiver(TracerSceneReceiverPlugin* m_TSRPlugin, QString _senderIP, bool m_debugState, zmq::context_t* m_context) {
        TSRPlugin = m_TSRPlugin;
        senderIP = _senderIP;
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;

        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req); // initialising the socket with a Request-Reply pattern

        // THIS TRIGGERS NULLPOINTER-RELATED EXCEPTION
        // open socket
        /*receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req);
        receiveSocket->connect(QString("tcp://" + ipAddress + ":5555").toLatin1().data());
        receiveSocket->setsockopt(ZMQ_REQ, "request scene", 0);*/
    }

    //! Constructor
    /*!
    * \param[in]    _senderIP       IP address for the socket connection
    * \param[in]    m_debugState    Whether the class should print out debug messages
    * \param[in]    m_context       A pointer to the 0MQ Context instanciated by \c m_TSRPlugin
    */
    SceneReceiver(QString _senderIP, bool m_debugState, zmq::context_t* m_context) {
        senderIP = _senderIP;
        _debug = m_debugState;
        context = m_context;
        _stop = true;
        _working = false;
        _paused = false;

        receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req);

        // THIS TRIGGERS NULLPOINTER-RELATED EXCEPTION
        // open socket
        /*receiveSocket = new zmq::socket_t(*context, zmq::socket_type::req);
        receiveSocket->connect(QString("tcp://" + ipAddress + ":5555").toLatin1().data());
        receiveSocket->setsockopt(ZMQ_REQ, "request scene", 0);*/
    };

    //! Default constructor
    ~SceneReceiver() {
        receiveSocket->close();
    };

    //! Request this process to start working
    /*!
    * Sets \c _working to true, \c _stopped and \c _paused to false and creates a new publishing \c sendSocket for message broadcasting
    */
    void requestStart() override {
        mutex.lock();
        _working = true;
        _stop = false;
        _paused = false;
        qDebug() << "AnimHost Scene Receiver requested to start";// in Thread "<<thread()->currentThreadId();
        mutex.unlock();
    };

    //! Request this process to stop working
    /*!
    * Sets \c _stopped to true and \c _paused to false. -Should- close \c sendSocket and cleanup
    */
    void requestStop() override {
        mutex.lock();
        if (_working) {
            _stop = true;
            _paused = false;
            //_working = false;
            qDebug() << "AnimHost Scene Receiver stopping";// in Thread "<<thread()->currentThreadId();
        }
        mutex.unlock();
    };

    //! Requesting the Character Package list in the TRACER client scene
    /*!
    * Uses a Request-Receive pattern to get the TRACER CharacterPackage data, converts message into explicit byte array and
    * passes byte array to TracerSceneReceiverPlugin to be parsed and saved in an AnimHost CharacterObjectSequence data structure
    */
    void requestCharacterData() {
        std::string charReq = "characters";
        requestMsg.rebuild(charReq.c_str(), charReq.size());
        qDebug() << "Requesting character packages";
        receiveSocket->send(requestMsg);

        receiveSocket->recv(&replyCharMsg);
        qDebug() << "Reply received!";

        QByteArray* charPkgArray = new QByteArray();
        // replyCharMsg will contain a series of bytes representing the Characters in the scene
        if (replyCharMsg.size() > 0)
            charPkgArray->append((char*)replyCharMsg.data(), replyCharMsg.size()); // Convert message into explicit byte array

        passCharacterByteArray(charPkgArray); // Pass byte array to TracerSceneReceiverPlugin to be parsed as chracter packages
    }

    //! Requesting the SceneNode list in the TRACER client scene
    /*!
    * Uses a Request-Receive pattern to get the TRACER SceneNode data, converts message into explicit byte array and
    * passes it to the TracerSceneReceiverPlugin to be parsed and saved in an AnimHost SceneNodeObjectSequence data structure
    */
    void requestSceneNodeData() {
        std::string nodesReq = "nodes";
        requestMsg.rebuild(nodesReq.c_str(), nodesReq.size());
        qDebug() << "Requesting nodes";
        receiveSocket->send(requestMsg);

        receiveSocket->recv(&replyNodeMsg);
        qDebug() << "Reply received!";

        QByteArray* nodesArray = new QByteArray();
        // replyNodeMsg will contain a series of bytes representing all the Scene Nodes in the scene
        if (replyNodeMsg.size() > 0)
            nodesArray->append((char*) replyNodeMsg.data(), replyNodeMsg.size());

        passSceneNodeByteArray(nodesArray); // Pass byte array to TracerSceneReceiverPlugin to be parsed as scene nodes
    }

    //! Requesting the header data containing global information of the TRACER client
    /*!
    * Uses a Request-Receive pattern to get the TRACER SceneNode data, converts message into explicit byte array and
    * passes it to the TracerSceneReceiverPlugin to be parsed and saved in an AnimHost SceneNodeObjectSequence data structure
    */
    void requestHeaderData() {
        std::string headReq = "header";
        requestMsg.rebuild(headReq.c_str(), headReq.size());
        qDebug() << "Requesting nodes";
        receiveSocket->send(requestMsg);

        receiveSocket->recv(&replyHeaderMsg);
        qDebug() << "Reply received!";

        QByteArray* headerArray = new QByteArray();
        // replyHeaderMsg will contain a series of bytes encoding basic global data of the scene and its client/server
        if (replyHeaderMsg.size() > 0)
            headerArray->append((char*) replyHeaderMsg.data(), replyHeaderMsg.size());

        passHeaderByteArray(headerArray); // Pass byte array to TracerSceneReceiverPlugin to be parsed as header data
    }

    //! Opens a Request-Receive communication socket
    /*!
    * \param[in]    newIPAddress    The (optional) new IP Address, empty string by default. If not empty the new address replaces the previous one
    */
    void connectSocket(QString newIPAddress = "") {
        //! (Re-)connect to newIPAddress
        /*if (receiveSocket->connected())
            receiveSocket->disconnect(QString("tcp://" + ipAddress + ":5555").toLatin1().data()); //disconnect throws unexpected exception*/
        if(!newIPAddress.isEmpty())
            senderIP = newIPAddress;

        receiveSocket->connect(QString("tcp://" + senderIP + ":5555").toLatin1().data());
    }

    private:
    zmq::socket_t* receiveSocket = nullptr; //!< Pointer to the instance of the socket that will receive the messages. It's going to be initialised in the constructor as a Request-Receive socket.
    QString senderIP;                       //!< The IP address, on which the connection is going to be established

    TracerSceneReceiverPlugin* TSRPlugin = nullptr; //!< Pointer to the instance of TracerSceneReceiverPlugin that owns this SceneReceiver
    zmq::message_t requestMsg {};                   //!< 0MQ message that will contain the request message to be sent out to DataHub or the TRACER client
    zmq::message_t replyNodeMsg {};                 //!< 0MQ message that will be filled with the scene description data
    zmq::message_t replyHeaderMsg {};               //!< 0MQ message that will be filled with the header data
    zmq::message_t replyCharMsg {};                 //!< 0MQ message that will be filled with the character list data

    public Q_SLOTS:
    //! Main loop. It doesn't do anything, since the class operations are supposed to be triggered directly from the UI of the Qt Application
    void run() {}

    Q_SIGNALS:
    void stopped();                                             //!< Signal emitted when process is finished
    void passCharacterByteArray(QByteArray* characterMsgArray); //!< Signal emitted to pass the character byte sequence onto the main thread
    void passSceneNodeByteArray(QByteArray* sceneNodeMsgArray); //!< Signal emitted to pass the scene node byte sequence onto the main thread
    void passHeaderByteArray(QByteArray* headerMsgArray);       //!< Signal emitted to pass the header byte sequence onto the main thread
};
#endif // SCENERECEIVER_H