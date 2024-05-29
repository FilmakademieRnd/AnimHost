
#ifndef PHASESEQUENCE_H
#define PHASESEQUENCE_H

#include "GNNPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>
#include "commondatatypes.h"
#include "FrameRange.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>



/**
* @class PhaseSequence
* 
* @brief Object that represents a sequence of phases and provides methods to manipulate them.
* 
*/

class GNNPLUGINSHARED_EXPORT PhaseSequence
{
	
	int sequenceLength = 120 + 1; // 120 frames + 1 pivot frame
	int sequencePivot = 60; // 60th frame is the pivot frame

	int numChannels = 5; // 5 channels for now, might change depending of complexity of animations

	FrameRange FullFrameWindow;
	FrameRange FutureFrameWindow;


	std::vector<std::vector<float>> phaseSequence;
	std::vector<std::vector<float>> frequencySequence;
	std::vector<std::vector<float>> amplitudeSequence;
	
public:

	PhaseSequence() : FullFrameWindow(13,60,60), FutureFrameWindow(13,60,60,6) {

		for (int frameIdx : FullFrameWindow) {
			qDebug() << "Frame Index: " << frameIdx;
		}
		
		for (int frameIdx : FutureFrameWindow) {
			qDebug() << "Frame Index: " << frameIdx;
		}

	    phaseSequence = std::vector<std::vector<float>>(sequenceLength, std::vector<float>(numChannels, 0.0f));
		frequencySequence = std::vector<std::vector<float>>(sequenceLength, std::vector<float>(numChannels, 1.f));
		amplitudeSequence = std::vector<std::vector<float>>(sequenceLength, std::vector<float>(numChannels, 1.f));
	};

	void IncrementSequence(int startIdx = 0, int endIdx = 60);
	void IncrementPastSequence();
	void UpdateSequence(const std::vector<std::vector<glm::vec2>>& newPhases, const std::vector<std::vector<float>>& newFrequencies,
		const std::vector<std::vector<float>>& newAmplitudes);	
	
	std::vector<glm::vec2> GetFlattenedPhaseSequence();

private:
	static glm::vec2 Calculate2dPhase(float phaseValue, float amplitude);

	static glm::vec2 Update2DPhase(float amplitude, float frequency, glm::vec2 current,
		glm::vec2 next, float minAmplitude = 0.f);

	static float CalcPhaseValue(glm::vec2 phase);
	
};

#endif // PHASESEQUENCE_H
