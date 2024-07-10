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

 
#include "TracerUpdateReceiverPlugin.h"
#include "TracerUpdateReceiver.h"

#include "../../core/commondatatypes.h"

TracerUpdateReceiverPlugin::TracerUpdateReceiverPlugin()
{
    qDebug() << "TracerUpdateReceiverPlugin created";

    _updateReceiverContext = new zmq::context_t(1);

    if (!msgReceiver) // trying to avoid multiple instances
        msgReceiver = new TracerUpdateReceiver(false, _updateReceiverContext);
    if (!zeroMQUpdateReceiverThread) // trying to avoid multiple instances
        zeroMQUpdateReceiverThread = new QThread();

    msgReceiver->moveToThread(zeroMQUpdateReceiverThread);
    QObject::connect(msgReceiver, &TracerUpdateReceiver::processUpdateMessage, this, &TracerUpdateReceiverPlugin::processParameterUpdate);
    QObject::connect(zeroMQUpdateReceiverThread, &QThread::started, msgReceiver, &TracerUpdateReceiver::run);

    qDebug() << "TracerUpdateSenderPlugin created";

    //Data
    //inputs.append(QMetaType(QMetaType::Int));
    //outputs.append(QMetaType(QMetaType::Float));
}

TracerUpdateReceiverPlugin::~TracerUpdateReceiverPlugin()
{
    qDebug() << "~TracerUpdateReceiverPlugin()";
}

unsigned int TracerUpdateReceiverPlugin::nDataPorts(QtNodes::PortType portType) const {
    if (portType == QtNodes::PortType::In)
        return 1;
    else
        return 1;
}

NodeDataType TracerUpdateReceiverPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    NodeDataType type;
    if (portType == QtNodes::PortType::In)  // INPUT Ports DataTypes
        if (portIndex == 0)
            return AnimNodeData<CharacterObjectSequence>::staticType();
        else
            return type;
    else                                    // OUTPUT Ports DataTypes 
        if (portIndex == 0)
            return AnimNodeData<ControlPath>::staticType();
        else
            return type;
}

void TracerUpdateReceiverPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) {
    if (!data) {
        switch (portIndex) {
            case 0:
                _characterListIn.reset();
                break;
            
            default:
                return;
        }
        return;
    }

    switch (portIndex) {
        case 0:_characterListIn = std::static_pointer_cast<AnimNodeData<CharacterObjectSequence>>(data);
            break;

        default:
            return;
    }

    qDebug() << "TracerUpdateReceiverPlugin setInData";
}

std::shared_ptr<NodeData> TracerUpdateReceiverPlugin::outData(QtNodes::PortIndex port) {
    return nullptr;
}

std::shared_ptr<NodeData> TracerUpdateReceiverPlugin::processOutData(QtNodes::PortIndex port) {
    if (port == 0)
        return _controlPathOut;
    else
        return nullptr;
}

bool TracerUpdateReceiverPlugin::isDataAvailable() {
    return !_characterListIn.expired();
}

void TracerUpdateReceiverPlugin::run() {
    qDebug() << "TracerUpdateReceiverPlugin running...";

    // start receiving thread
    if (!zeroMQUpdateReceiverThread->isRunning()) {
        msgReceiver->requestStart();
        zeroMQUpdateReceiverThread->start();
    }

}

void TracerUpdateReceiverPlugin::processParameterUpdate(QByteArray* paramUpdateMsg) {

}
