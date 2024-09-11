#include "TRACERUpdateMessage.h"
#include <QDataStream>

// Define glm::vec3 serialization
QDataStream& operator>>(QDataStream& stream, glm::vec3& vec) {
    stream >> vec.x;
    stream >> vec.y;
    stream >> vec.z;
    return stream;
}

// Define glm::quat serialization
QDataStream& operator>>(QDataStream& stream, glm::quat& quat) {
	stream >> quat.w;
	stream >> quat.x;
	stream >> quat.y;
	stream >> quat.z;
	return stream;
}
