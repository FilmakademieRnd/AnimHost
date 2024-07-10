
#ifndef ROOTSERIES_H
#define ROOTSERIES_H



#include "GNNPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>
#include "commondatatypes.h"
#include "TimeSeries.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

/*
Program Flow:

1. Init Series with initial root transform (from control path) and zero velocity  [x]

Loop:
1. Increment full sequence  (end samples-1) [x]
2. Apply Controls
	a. leave past samples as is [x]
	b. for pivot and future mix with control path (see starke Control() function) [x]
3. Prepare Network Input
	a. Extract keys relative to current root (root should match transform in pivotIndex-1) [ ]
	b. Position, Direction, velocity and speed of each key [ ]

4. Run Network...

5. Update future series 
	a. Update root and velocity with network delta output [x]
	b. apply on pivot index [ ]
	c. calc future transforms and velocities with network output from new root (results in global transforms) [x]
	d. apply to corresponding indices in series (mix with existing values) [x]
	f. interpolate values between pivot and future keys [ ]...
   


*/



class GNNPLUGINSHARED_EXPORT RootSeries : public TimeSeries
{

	std::vector<glm::mat4> transforms;
	std::vector<glm::vec3> velocities;

public:
	RootSeries() : TimeSeries() {
	}

	void Setup(glm::mat4 rootTransform) {
		transforms = std::vector<glm::mat4>(this->numSamples, rootTransform);
		velocities = std::vector<glm::vec3>(this->numSamples, glm::vec3(0.0f));
	}

	void UpdateTransform(glm::mat4 updatedRoot, int sampleIdx) {
		transforms[sampleIdx] = updatedRoot;
	}

	void UpdateVelocity(glm::vec3 updatedVelocity, int sampleIdx) {
		velocities[sampleIdx] = updatedVelocity;
	}


	void UpdatePivotRoot(glm::mat4 updatedRoot) {
		transforms[this->pivotIndex] = updatedRoot;
	}

	void UpdatePivotVelocity(glm::vec3 updatedVelocity) {
		velocities[this->pivotIndex] = updatedVelocity;
	}

	void ApplyControls(const std::vector<glm::vec2>& ctrlPositions, const std::vector<glm::quat> ctrlOrientations, const std::vector<glm::vec2> ctrlVelocities, float tau = 1.f);

	void UpdateFutureRootSeries(const std::vector<glm::mat4>& futureTransforms, const std::vector<glm::vec3>& futureVelocities);

	void IncrementSequence(int startIdx, int endIdx) override;

	void Interpolate(int startIdx, int endIdx) override;

	glm::vec3 GetPosition(int idx) const {
		return glm::vec3(transforms[idx][3]);
	}

	glm::quat GetRotation(int idx) const {
		return glm::toQuat(transforms[idx]);
	}

	glm::vec3 GetVelocity(int idx) const {
		return velocities[idx];
	}

	glm::mat4 GetTransform(int idx) const {
		return transforms[idx];
	}

	std::vector<glm::mat4> GetTransforms() const {
		return transforms;
	}
};

#endif // ROOTSERIES_H