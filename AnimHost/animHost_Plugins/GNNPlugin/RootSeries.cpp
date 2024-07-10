#include "RootSeries.h"
#include <algorithm>

void RootSeries::ApplyControls(const std::vector<glm::vec2>& ctrlPositions, const std::vector<glm::quat> ctrlOrientations, const std::vector<glm::vec2> ctrlVelocities, float tau)
{
	IncrementSequence(0, numSamples-1);


	//creat local copies of the current positions, rotations and velocities
	std::vector<glm::vec3> tempPositions(numSamples);
	std::transform(transforms.begin(), transforms.end(), tempPositions.begin(), [&](glm::mat4 t) {
		return t * glm::vec4(0, 0, 0, 1);
		});

	std::vector<glm::quat> tempRotations(numSamples);
	std::transform(transforms.begin(), transforms.end(), tempRotations.begin(), [&](glm::mat4 t) {
		return glm::toQuat(t);
		});

	std::vector<glm::vec3> tempVelocities = velocities;


	//apply the controls from pivot onwards
	for (int i = pivotIndex; i < numSamples; i++)
	{
		float weight = (i - pivotIndex) / float(numSamples- pivotIndex);
		weight = glm::pow(weight, tau);

		tempPositions[i] = glm::mix(tempPositions[i], glm::vec3(ctrlPositions[i - pivotIndex].x, 0.f, ctrlPositions[i - pivotIndex].y), weight);

		tempRotations[i] = glm::slerp(tempRotations[i], ctrlOrientations[i - pivotIndex], weight);
		//Velocity currently not controlled need refinement
		tempVelocities[i] = glm::mix(tempVelocities[i], glm::vec3(ctrlVelocities[i - pivotIndex].x, 0.f, ctrlVelocities[i - pivotIndex].y), weight);
	}

	
	for(int i = 0; i < numSamples; i++)
	{
		transforms[i] = glm::translate(glm::mat4(1.0f), tempPositions[i]) * glm::toMat4(tempRotations[i]);
		velocities[i] = tempVelocities[i];
	}
}

void RootSeries::UpdateFutureRootSeries(const std::vector<glm::mat4>& futureTransforms, const std::vector<glm::vec3>& futureVelocities)
{
}

void RootSeries::IncrementSequence(int startIdx, int endIdx)
{
	for (int i = startIdx; i < endIdx; i++)
	{
		transforms[i] = transforms[i+1];
		velocities[i] = velocities[i+1];
	}
}

void RootSeries::Interpolate(int startIdx, int endIdx)
{


	for (int i = startIdx; i < endIdx; i++)
	{
		float weight = (i % 10) / 10.f;
		int prevKey = glm::floor(i / 10.f) * 10;
		int nextKey = glm::ceil(i / 10.f) * 10;

		if (prevKey != nextKey) {
			glm::vec3 newPos = glm::mix(transforms[prevKey] * glm::vec4(0, 0, 0, 1), transforms[nextKey] * glm::vec4(0, 0, 0, 1), weight);
			glm::quat newRot = glm::slerp(glm::toQuat(transforms[prevKey]), glm::toQuat(transforms[nextKey]), weight);
			
			transforms[i] = glm::translate(glm::mat4(1.0f), newPos) * glm::toMat4(newRot);

			velocities[i] = glm::mix(velocities[prevKey], velocities[nextKey], weight);
		}
	
	};

	qDebug() << "Interpolating";
}



