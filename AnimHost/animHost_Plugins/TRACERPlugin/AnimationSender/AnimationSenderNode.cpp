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

#include "AnimationSenderNode.h"
#include "AnimHostMessageSender.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

AnimationSenderNode::AnimationSenderNode(std::shared_ptr<TRACERGlobalTimer> globalTimer, std::shared_ptr<zmq::context_t> zmqConext) : 
    _globalTimer(globalTimer), _updateSenderContext(zmqConext)
{
    _sendStreamButton = nullptr;
    widget = nullptr;
    _selectIPAddress = nullptr;
    _loopCheck = nullptr;
    _ipAddressLayout = nullptr;
    _ipAddress = nullptr;
    _ipValidator = nullptr;

    if (!msgSender) // trying to avoid multiple instances
        msgSender = new AnimHostMessageSender(false, _updateSenderContext.get(), _globalTimer);
    if (!zeroMQSenderThread) // trying to avoid multiple instances
        zeroMQSenderThread = new QThread();

    msgSender->moveToThread(zeroMQSenderThread);

    connect(zeroMQSenderThread, &QThread::started, msgSender, &AnimHostMessageSender::run);
    connect(msgSender, &AnimHostMessageSender::stopped, zeroMQSenderThread, &QThread::quit);
    connect(zeroMQSenderThread, &QThread::finished, msgSender, &QObject::deleteLater);
    connect(zeroMQSenderThread, &QThread::finished, zeroMQSenderThread, &QObject::deleteLater);


    qDebug() << "AnimationSenderNode created";
}

AnimationSenderNode::~AnimationSenderNode()
{

    if (zeroMQSenderThread->isRunning()) {
        msgSender->requestStop();
        zeroMQSenderThread->quit();
        zeroMQSenderThread->wait();
        qDebug() << "AnimationSenderNode: TickReceiverThread stopped";
    }

    qDebug() << "~AnimationSenderNode()";
}

unsigned int AnimationSenderNode::nDataPorts(QtNodes::PortType portType) const {
    if (portType == QtNodes::PortType::In)
        return 3;
    else
        return 0;
}

NodeDataType AnimationSenderNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    NodeDataType type;
    if (portType == QtNodes::PortType::In)  // INPUT Ports DataTypes
        if (portIndex == 0)
            return AnimNodeData<Animation>::staticType();
        else if (portIndex == 1)
            return AnimNodeData<CharacterObject>::staticType();
        else if (portIndex == 2)
            return AnimNodeData<SceneNodeObjectSequence>::staticType();
        else
            return type;
    else                                    // OUTPUT Ports DataTypes 
        return type;
}

void AnimationSenderNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) {

    if (!data) {
        switch (portIndex) {
        case 0:
            _animIn.reset();
            break;
        case 1:
            _characterIn.reset();
            break;
        case 2:
            _sceneNodeListIn.reset();
            break;

        default:
            return;
        }
        return;
    }

    switch (portIndex) {
    case 0:
        _animIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
        break;
    case 1:
        _characterIn = std::static_pointer_cast<AnimNodeData<CharacterObject>>(data);
        break;
    case 2:
        _sceneNodeListIn = std::static_pointer_cast<AnimNodeData<SceneNodeObjectSequence>>(data);
        break;

    default:
        return;
    }

    qDebug() << "AnimationSenderNode setInData";
}

bool AnimationSenderNode::isDataAvailable() {
    return !_animIn.expired() && !_characterIn.expired() && !_sceneNodeListIn.expired();
}

void AnimationSenderNode::run() {
    if (!isDataAvailable())
        return;

    qDebug() << "AnimationSenderNode running...";

    if (!zeroMQSenderThread->isRunning()) {
        msgSender->requestStart();
        zeroMQSenderThread->start();
    }



    // TEMP: SEND ANIMATION EN BLOC EVERY TIME THE NODE IS RUN
    // IGNORE USER SETTING IN THE WIDGET
    if (isStreaming) {
        // Stop Streaming

        _sendStreamButton->setText("Send Animation");
        isStreaming = false;

        msgSender->setStreamAnimation(AnimHostMessageSender::STREAMSTOP);

    }

    if (isDataAvailable()) {
        auto sp_character = _characterIn.lock();
        auto sp_animation = _animIn.lock();
        auto sp_sceneNodeList = _sceneNodeListIn.lock();

        // Set animation, character and scene data (necessary for creating a pose update message) in the message sender object
        msgSender->setAnimationAndSceneData(sp_animation->getData(), sp_character->getData(), sp_sceneNodeList->getData());

        // Start Streaming
        msgSender->setStreamAnimation(AnimHostMessageSender::ENBLOCK);
    }



    // every time the buffer is **predicted to be** half-read, fill the next half buffer
    // TO BE UNCOMMENTED AS SOON AS I RECEIVE bufferSize and renderingFrameRate from receiver
    /*if (localTime % (bufferReadTime / 2) == 0) {
        msgSender->resume();
    }*/
}

std::shared_ptr<NodeData> AnimationSenderNode::processOutData(QtNodes::PortIndex port) {
    return nullptr;
}

QWidget* AnimationSenderNode::embeddedWidget() {
    if (!widget) {

        widget = new QWidget();
        _mainLayout = new QVBoxLayout();

        //IP Address Selection
        _ipAddressLayout = new QHBoxLayout();
        _selectIPAddress = new QComboBox();
        _ipAddress = ZMQMessageHandler::getIPList().at(0).toString();
        _ipValidator = new QRegularExpressionValidator(ZMQMessageHandler::ipRegex, this);
        _selectIPAddress->setValidator(_ipValidator);

        for (QHostAddress ipAddress : ZMQMessageHandler::getIPList()) {
            _selectIPAddress->addItem(ipAddress.toString());
        }

        _ipAddressLayout->addWidget(_selectIPAddress);
        _ipAddressLayout->setSizeConstraint(QLayout::SetMinimumSize);
        _mainLayout->addLayout(_ipAddressLayout);

        connect(_selectIPAddress, &QComboBox::currentIndexChanged, this, &AnimationSenderNode::onChangedSelection);

        // Toggle between En Bloc and Stream Animation
        _streamCheck = new QCheckBox("Stream");
        _mainLayout->addWidget(_streamCheck);

        connect(_streamCheck, &QCheckBox::stateChanged, [this](int state) {
            if (state == Qt::Checked) {
                _streamWidget->show();
                _enBlocWidget->hide();

            }
            else {
                _streamWidget->hide();
                _enBlocWidget->show();
            }

            widget->adjustSize();
            Q_EMIT embeddedWidgetSizeUpdated();
            });

        // Stream Animation Section
        {
            _streamWidget = new QWidget();
            _streamLayout = new QHBoxLayout();
            _sendStreamButton = new QPushButton("Send Animation");
            _sendStreamButton->resize(QSize(30, 30));
            _loopCheck = new QCheckBox("Loop");

            _streamLayout->addWidget(_loopCheck);
            _streamLayout->addWidget(_sendStreamButton);

            _streamWidget->setLayout(_streamLayout);

            _mainLayout->addWidget(_streamWidget);

            _streamWidget->hide();

            connect(_loopCheck, &QCheckBox::stateChanged, this, &AnimationSenderNode::onLoopCheck);
            connect(_sendStreamButton, &QPushButton::released, this, &AnimationSenderNode::onStreamButtonClicked);
        }

        // En Bloc Animation Section
        {
            _enBlocWidget = new QWidget();
            _enBlocLayout = new QHBoxLayout();

            _sendEnBlocButton = new QPushButton("Send En Bloc");

            _enBlocLayout->addWidget(_sendEnBlocButton);

            _enBlocWidget->setLayout(_enBlocLayout);

            _mainLayout->addWidget(_enBlocWidget);

            connect(_sendEnBlocButton, &QPushButton::released, this, &AnimationSenderNode::onEnBlocButtonClicked);
        }



        widget->setLayout(_mainLayout);
        widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
            "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
            "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
            "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
        );

    }

    return widget;
}

void AnimationSenderNode::onChangedSelection(int index) {
    qDebug() << "IP Address Selection Changed";
    if (index >= 0) {
        ZMQMessageHandler::setIPAddress(ZMQMessageHandler::getIPList().at(index).toString());
        emitDataUpdate(0);
    }
    else {
        emitDataInvalidated(0);
    }
    qDebug() << "New IP Address:" << ZMQMessageHandler::getOwnIP();
}

void AnimationSenderNode::onLoopCheck(int state) {
    // loop is false when checkbox is unchecked, true otherwise
    msgSender->loop = (state != Qt::Unchecked);
}

void AnimationSenderNode::onStreamButtonClicked() {

    // Start/Stop streaming of animation

    if (isStreaming) {

        // Stop Streaming

        _sendStreamButton->setText("Send Animation");
        isStreaming = false;
        msgSender->setStreamAnimation(AnimHostMessageSender::STREAMSTOP);


    }
    else {

        if (isDataAvailable()) {
            auto sp_character = _characterIn.lock();
            auto sp_animation = _animIn.lock();
            auto sp_sceneNodeList = _sceneNodeListIn.lock();

            // Set animation, character and scene data (necessary for creating a pose update message) in the message sender object
            msgSender->setAnimationAndSceneData(sp_animation->getData(), sp_character->getData(), sp_sceneNodeList->getData());

            _sendStreamButton->setText("Stop Animation");
            isStreaming = true;
            msgSender->setStreamAnimation(AnimHostMessageSender::STREAMSTART);
        }


    }

}

void AnimationSenderNode::onEnBlocButtonClicked()
{
    //msgSender->setStreamAnimation(false);

    if (isStreaming) {
        // Stop Streaming

        _sendStreamButton->setText("Send Animation");
        isStreaming = false;

        msgSender->setStreamAnimation(AnimHostMessageSender::STREAMSTOP);

    }

    if (isDataAvailable()) {
        auto sp_character = _characterIn.lock();
        auto sp_animation = _animIn.lock();
        auto sp_sceneNodeList = _sceneNodeListIn.lock();

        // Set animation, character and scene data (necessary for creating a pose update message) in the message sender object
        msgSender->setAnimationAndSceneData(sp_animation->getData(), sp_character->getData(), sp_sceneNodeList->getData());

        // Start Streaming
        msgSender->setStreamAnimation(AnimHostMessageSender::ENBLOCK);
    }

}