
#ifndef TRACERUPDATEMESSAGE_H
#define TRACERUPDATEMESSAGE_H


#include "TRACERPlugin_global.h"
#include <commondatatypes.h>
#include <ZMQMessageHandler.h>


class TRACERPLUGINSHARED_EXPORT UpdateMessage {

public:
    uint8_t sceneID;
    uint16_t objectID;
    uint16_t paramID;
    ZMQMessageHandler::ParameterType paramType;
    QByteArray rawData;

    UpdateMessage(uint8_t sceneID, uint16_t objectID, uint16_t paramID,
        ZMQMessageHandler::ParameterType paramType, const QByteArray& rawData)
        : sceneID(sceneID), objectID(objectID), paramID(paramID), paramType(paramType), rawData(rawData) {};


};

class TRACERPLUGINSHARED_EXPORT ParameterUpdate : public UpdateMessage {

public:

    ParameterUpdate() : UpdateMessage(0, 0, 0, ZMQMessageHandler::ParameterType::UNKNOWN, QByteArray()) {};

    ParameterUpdate(uint8_t sceneID, uint16_t objectID, uint16_t paramID,
        ZMQMessageHandler::ParameterType paramType, const QByteArray rawData) :
        UpdateMessage(sceneID, objectID, paramID, paramType, rawData) {};


    COMMONDATA(parameterUpdate, ParameterUpdate)
};
Q_DECLARE_METATYPE(ParameterUpdate)
Q_DECLARE_METATYPE(std::shared_ptr<ParameterUpdate>)

class TRACERPLUGINSHARED_EXPORT RPCUpdate : public UpdateMessage {

public:

    RPCUpdate() : UpdateMessage(0, 0, 0, ZMQMessageHandler::ParameterType::UNKNOWN, QByteArray()) {};

    RPCUpdate(uint8_t sceneID, uint16_t objectID, uint16_t paramID,
        ZMQMessageHandler::ParameterType paramType, const QByteArray rawData) :
        UpdateMessage(sceneID, objectID, paramID, paramType, rawData) {};

    RPCUpdate(const RPCUpdate& other) : UpdateMessage(other.sceneID, other.objectID, other.paramID, other.paramType, other.rawData) {};


    COMMONDATA(rpcUpdate, RPCUpdate)
};
Q_DECLARE_METATYPE(RPCUpdate)
Q_DECLARE_METATYPE(std::shared_ptr<RPCUpdate>)

#endif // !TRACERUPDATEMESSAGE_H