#ifndef MATHUTILS_H
#define MATHUTILS_H

#include "animhostcore_global.h"
#include "commondatatypes.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

/**
 * @class MathUtils
 * *
 * @brief A class that provides utility functions for mathematical operations.
 * */
class MathUtils {

public:

	static glm::vec2 PositionTo(const glm::vec2& from, const glm::mat4& to) {
		glm::vec4 pos = glm::inverse(to) * glm::vec4(from.x, 0.0f, from.y, 1.0f);
		return glm::vec2(pos.x, pos.z);
	}

	static glm::vec2 PositionTo(const glm::mat4& from, const glm::mat4& to) {
		glm::vec4 pos = glm::inverse(to) * from * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		return glm::vec2(pos.x, pos.z);
	}


	static glm::vec3 PositionTo(const glm::vec3& from, const glm::mat4& to) {
		glm::vec4 pos = glm::inverse(to) * glm::vec4(from.x, 0.0, from.y, 1.0f);
		return glm::vec3(pos.x, pos.y, pos.z);
	}

	static glm::vec2 ForwardTo(const glm::mat4& from, const glm::mat4& to) {
		glm::vec3 dir = glm::inverse(to) * from * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		return glm::normalize(glm::vec2(dir.x, dir.z));
	}



	static glm::vec3 DirectionTo(const glm::vec3& from, const glm::mat4& to) {
		glm::vec4 dir = glm::inverse(to) * glm::vec4(from, 0.0f);
		return glm::normalize(glm::vec3(dir.x, dir.y, dir.z));
	}

	static glm::vec2 DirectionTo(const glm::vec2& from, const glm::mat4& to) {
		glm::vec4 dir = glm::inverse(to) * glm::vec4(from.x, 0.0f, from.y, 0.0f);
		return glm::normalize(glm::vec2(dir.x, dir.z));
	}

	static glm::vec3 VelocityTo(const glm::vec3& from, const glm::mat4& to) {
		glm::vec4 dir = glm::inverse(to) * glm::vec4(from, 0.0f);
		return glm::vec3(dir.x, dir.y, dir.z);
	}

	static glm::quat DecomposeRotation(const glm::mat4& matrix) {
		glm::vec3 scale, skew, translation;
		glm::vec4 perspective;
		glm::quat rotation;
		glm::decompose(matrix, scale, rotation, translation, skew, perspective);
		glm::quat conRotation = glm::conjugate(rotation);
		
		return conRotation;
	}
};


#endif // !MATHUTILS_H