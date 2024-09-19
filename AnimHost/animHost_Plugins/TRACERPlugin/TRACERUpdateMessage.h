
#ifndef TRACERUPDATEMESSAGE_H
#define TRACERUPDATEMESSAGE_H


#include "TRACERPlugin_global.h"
#include <commondatatypes.h>
#include <ZMQMessageHandler.h>
#include <QDataStream>


// VECTOR3
inline TRACERPLUGINSHARED_EXPORT QDataStream& operator>>(QDataStream& stream, glm::vec3& vec) { stream >> vec.x; stream >> vec.y; stream >> vec.z; return stream; }

// VECTOR4
inline TRACERPLUGINSHARED_EXPORT QDataStream& operator>>(QDataStream& stream, glm::vec4& vec){ stream >> vec.x; stream >> vec.y; stream >> vec.z; stream >> vec.w; return stream; }

//QUATERNION
inline TRACERPLUGINSHARED_EXPORT QDataStream& operator>>(QDataStream& stream, glm::quat& quat) { stream >> quat.w; stream >> quat.x; stream >> quat.y; stream >> quat.z; return stream; }

template <typename T>
struct TRACERPLUGINSHARED_EXPORT KeyFrame {

    float time;
    T value;

    uint8_t keyType;

    float inTangentTime;
    T inTangentValue;

    float outTangentTime;
    T outTangentValue;

    KeyFrame() {};

    //Constructor with optional tangent values
    KeyFrame(float time, T value, uint8_t keyType, float inTangentTime = 0.0, T inTangentValue = 0, float outTangentTime = 0.0, T outTangentValue = 0) :
		time(time), value(value), keyType(keyType), inTangentTime(inTangentTime), inTangentValue(inTangentValue), outTangentTime(outTangentTime), outTangentValue(outTangentValue) {};

};



class TRACERPLUGINSHARED_EXPORT AbstractParameterPayload {

public:
    AbstractParameterPayload() {};
    virtual  ~AbstractParameterPayload() = default;

};

template <typename T>
class TRACERPLUGINSHARED_EXPORT ParameterPayload : public AbstractParameterPayload {
    
    T parameterValue;

    std::vector<KeyFrame<T>> _keyList;

    public:

    ParameterPayload(const T& value, const std::vector<KeyFrame<T>>& keyList) : parameterValue(value), _keyList(keyList) {};

    ParameterPayload(const T& value) : parameterValue(value) {};

    ParameterPayload(QDataStream& stream) {
		
        
        stream >> parameterValue;

        if (stream.atEnd()) {
            return;
        }

        uint16_t keyListSize;
        stream >> keyListSize;

        for (uint32_t i = 0; i < keyListSize; i++) {
			KeyFrame<T> keyFrame;

            stream >> keyFrame.keyType;
			stream >> keyFrame.time;
            stream >> keyFrame.inTangentTime;
            stream >> keyFrame.outTangentTime;
			stream >> keyFrame.value;
            stream >> keyFrame.inTangentValue;
            stream >> keyFrame.outTangentValue;

			_keyList.push_back(keyFrame);
		}

	};

    T getValue() { return parameterValue; }
    std::vector<KeyFrame<T>> getKeyList() { return _keyList; }

};



class TRACERPLUGINSHARED_EXPORT UpdateMessage {

public:

    uint8_t sceneID; /**< The scene id associated with the update. */

    uint16_t objectID; /**< The object id associated with the update. */
    uint16_t paramID; /**< The parameter id associated with the update. */
    ZMQMessageHandler::ParameterType paramType; /**< The parameter type associated with the update. */
    
    /**
     * The raw payload data associated with the update.
	 * The raw data contains the payload of the update message.
	 * Data starts from the parameter value. Excluding the header.
	 * May contain adiitional key data if the parameter is animated.
     */
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

   
    // decoder with void pointer parameter return values   
    std::unique_ptr<AbstractParameterPayload> decodeRawData() {
		//Decode Raw Data

        QDataStream stream(rawData);
        stream.setByteOrder(QDataStream::LittleEndian); // Assuming little-endian, adjust if needed
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        switch (paramType) {
        case ZMQMessageHandler::ParameterType::INT:
            qDebug() << "Decoding INT";
            return std::make_unique<ParameterPayload<int>>(stream);
            break;
        case ZMQMessageHandler::ParameterType::FLOAT:
            qDebug() << "Decoding FLOAT";
            return std::make_unique<ParameterPayload<float>>(stream);
            break;
        case ZMQMessageHandler::ParameterType::VECTOR3:
            qDebug() << "Decoding VECTOR3";
            return std::make_unique<ParameterPayload<glm::vec3>>(stream);
            break;
        case ZMQMessageHandler::ParameterType::VECTOR4:
            qDebug() << "Decoding VECTOR4";
            return std::make_unique<ParameterPayload<glm::vec4>>(stream);
            break;
        case ZMQMessageHandler::ParameterType::QUATERNION:
            qDebug() << "Decoding QUATERNION";
            return std::make_unique<ParameterPayload<glm::quat>>(stream);
            break;
        default:
            qDebug() << "Unsupported parameter type!";
            return std::make_unique<AbstractParameterPayload>();
            break;
        }
	}


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