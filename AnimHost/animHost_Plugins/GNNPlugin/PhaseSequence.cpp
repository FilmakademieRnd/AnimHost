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

 
#include "PhaseSequence.h"

#include "MathUtils.h"
#include <glm/gtx/vector_angle.hpp>

void PhaseSequence::IncrementSequence(int startIdx, int endIdx)
{

	for(int i = startIdx; i < endIdx; i++)
	{
		for(int j = 0; j< numChannels; j++)
		{
			phaseSequence[i][j] = phaseSequence[i+1][j];
			frequencySequence[i][j] = frequencySequence[i+1][j];
			amplitudeSequence[i][j] = amplitudeSequence[i+1][j];
		}
	}
}

void PhaseSequence::IncrementPastSequence() {
	IncrementSequence(0, sequencePivot);
}

void PhaseSequence::UpdateSequence(const std::vector<std::vector<glm::vec2>>& newPhases, const std::vector<std::vector<float>>& newFrequencies, const std::vector<std::vector<float>>& newAmplitudes, float phaseBias)
{

	float minAmplitude = 0.01f;

	int inIdx = 0;

	for(int index : FutureFrameWindow)
	{
		for (int channel = 0; channel < numChannels; channel++)
		{

			glm::vec2 current = Calculate2dPhase(phaseSequence[index][channel], 1.0f);

			glm::vec2 next = glm::normalize(newPhases[inIdx][channel]);


			float amplitude = glm::abs(newAmplitudes[inIdx][channel]);
			amplitude = glm::max(amplitude, minAmplitude);

			float frequency = glm::abs(newFrequencies[inIdx][channel]);

			//rotate current by angle in degrees
			float degrees = - frequency * 360.f * (1.f / 60.f);

			glm::vec2 updated = glm::rotateZ(glm::vec3(current, 0.f), glm::radians(degrees));
			updated = glm::normalize(updated);

			

			float a = glm::orientedAngle(glm::vec2(0.f, 1.f), updated);
			float b = glm::orientedAngle(glm::vec2(0.f, 1.f), next);

			glm::quat updatedQuat = glm::quat(glm::vec3(0.f, 1.f, 0.f), glm::vec3(updated,0.f));
			glm::quat nextQuat = glm::quat(glm::vec3(0.f, 1.f, 0.f), glm::vec3(next,0.f));
			

			glm::quat mixQuat = glm::slerp(updatedQuat, nextQuat, phaseBias);

			glm::vec3 mixedVec3 = mixQuat * glm::vec3(0.f, 1.f, 0.f);

			glm::vec2 mixed = glm::vec2(mixedVec3.x, mixedVec3.y);


			phaseSequence[index][channel] = CalcPhaseValue(mixed);
			frequencySequence[index][channel] = frequency;
			amplitudeSequence[index][channel] = amplitude;
			
		}

		++inIdx;
	}
}

std::vector<glm::vec2> PhaseSequence::GetFlattenedPhaseSequence()
{

	std::vector<glm::vec2> flattenPhaseSequence;

	for(int frame: FullFrameWindow)
	{
		for(int channel = 0; channel < numChannels; channel++)
		{
			flattenPhaseSequence.push_back(Calculate2dPhase(phaseSequence[frame][channel], amplitudeSequence[frame][channel]));
		}
	}

	return flattenPhaseSequence;
}



glm::vec2 PhaseSequence::Calculate2dPhase(float phaseValue, float amplitude)
{
	phaseValue *= (2.f * glm::pi<float>());

	float x_val = glm::sin(phaseValue) * amplitude;
	float y_val = glm::cos(phaseValue) * amplitude;

	return glm::vec2(x_val, y_val);
}

glm::vec2 PhaseSequence::Update2DPhase(float amplitude, float frequency, glm::vec2 current, glm::vec2 next, float minAmplitude)
{
	amplitude = glm::abs(amplitude);
	amplitude = glm::max(amplitude, minAmplitude);

	frequency = glm::abs(frequency);

	glm::vec2 updated = glm::angleAxis(-frequency * 360.f * (1.f / 60.f), glm::vec3(0.f, 0.f, 1.f)) * glm::vec3(current, 0.0f);

	next = glm::normalize(next);
	updated = glm::normalize(updated);

	//slerp work around
	glm::quat a = glm::quat(glm::vec3(next, 0.f), glm::vec3(0.f, 0.f, 1.f));
	glm::quat b = glm::quat(glm::vec3(updated, 0.f), glm::vec3(0.f, 0.f, 1.f));
	glm::quat mix = glm::slerp(a, b, 0.5f);

	auto mixed = mix * glm::vec3(0.f, 0.f, 1.f);

	return glm::vec2(mixed);
}

float PhaseSequence::CalcPhaseValue(glm::vec2 phase2D)
{
	float angle = -glm::orientedAngle(glm::vec2(0.f, 1.f), glm::normalize(phase2D));

	
	angle = glm::degrees(angle);
	
	if (angle < 0.f) {
		angle = 360.f + angle;
	}

	float phase = glm::mod(angle / 360.f, 1.f);
	return phase;
}
