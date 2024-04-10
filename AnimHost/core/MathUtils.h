#ifndef MATHUTILS_H
#define MATHUTILS_H

#include "animhostcore_global.h"
#include "commondatatypes.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

using Rotation6D = std::vector<float>;

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
		glm::vec4 pos = glm::inverse(to) * glm::vec4(from, 1.0f);
		return glm::vec3(pos.x, pos.y, pos.z);
	}

	static glm::vec2 ForwardTo(const glm::mat4& from, const glm::mat4& to) {
		glm::vec3 dir = glm::inverse(to) * from * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		return glm::normalize(glm::vec2(dir.x, dir.z));
	}

	static glm::mat4 RelativeTransform(const glm::mat4& from, const glm::mat4& to) {
		return glm::inverse(to) * from;
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
		//glm::quat conRotation = glm::conjugate(rotation);
		
		//return conRotation;
		return rotation;
	}

	static Rotation6D ConvertRotationTo6D(const glm::quat& rotation) {
		std::vector<float> result;
		
		glm::mat4 rotMat = glm::toMat4(rotation);

		result.push_back(rotMat[0].x);
		result.push_back(rotMat[0].y);
		result.push_back(rotMat[0].z);
		result.push_back(rotMat[1].x);
		result.push_back(rotMat[1].y);
		result.push_back(rotMat[1].z);

		return result;
	}

	static std::vector<Rotation6D> convertQuaternionsTo6DRotations(const std::vector<glm::quat>& quaternions) {
		std::vector<Rotation6D> rotations6D;

		std::transform(quaternions.begin(), quaternions.end(), std::back_inserter(rotations6D), [](const glm::quat& quaternion) {
			return MathUtils::ConvertRotationTo6D(quaternion);
			});

		return rotations6D;
	}


	/**
	 * @brief Converts a 6D rotation vector into a quaternion.
	 *
	 * This method is based on the method described in the paper "On the Continuity of Rotation Representations in Neural Networks".
	 *
	 * @param rotation6D A vector of 6 floats representing the 6D rotation.
	 * @return A quaternion representing the same rotation as the input 6D rotation vector.
	 */
	static glm::quat Convert6DToRotation(const Rotation6D& rotation6D) {

		//check for size 6
		if (rotation6D.size() != 6) {\
			qDebug() << "Error: Invalid 6D rotation size";
			return glm::quat();
		}

		// Create two 3D vectors from the first three and the last three elements of the input vector.
		glm::vec3 a1 = glm::vec3(rotation6D[0], rotation6D[1], rotation6D[2]);
		glm::vec3 a2 = glm::vec3(rotation6D[3], rotation6D[4], rotation6D[5]);

		// Normalize the first vector.
		glm::vec3 b1 = glm::normalize(a1);

		// Create another vector that is orthogonal to the first vector.
		glm::vec3 b2 = glm::normalize(a2 - glm::dot(b1, a2) * b1);

		// Create a third vector that is orthogonal to both the first and the second vectors.
		glm::vec3 b3 = glm::cross(b1, b2);

		// Create a 3x3 rotation matrix from the vectors.
		glm::mat3 rotMat;
		rotMat[0] = b1;
		rotMat[1] = b2;
		rotMat[2] = b3;

		// Convert the rotation matrix into a quaternion and return it.
		return glm::quat_cast(rotMat);
	}


	static std::vector<glm::quat> convert6DRotationToQuaternions(const std::vector<Rotation6D>& rotations6d) {
		std::vector<glm::quat> quaternions;

		std::transform(rotations6d.begin(), rotations6d.end(), std::back_inserter(quaternions), [](const Rotation6D& rotation6D) {
			return MathUtils::Convert6DToRotation(rotation6D);
			});

		return quaternions;
	}


	static glm::vec2 rotateVec2(const glm::vec2& v, float deg) {
		float rad = glm::radians(deg);
		glm::mat2 rotMatrix = glm::mat2(glm::cos(rad), -glm::sin(rad),
			                            glm::sin(rad), glm::cos(rad));
		return rotMatrix * v;
	}
};


#endif // !MATHUTILS_H