#ifndef TIMESERIES_H
#define TIMESERIES_H

#include "animhostcore_global.h"
#include "commondatatypes.h"

#include "FrameRange.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>



class TimeSeries
{
	protected:
	int numSamples;
	int numKeys;
	int pivotIndex;

	FrameRange FullFrameWindow;
	FrameRange FutureFrameWindow;

	int pastKeys;
	int futureKeys;
	int resolution;




public:

	TimeSeries() : pastKeys(6), futureKeys(6), resolution(10), FullFrameWindow(13, 60, 60), FutureFrameWindow(13, 60, 60, 6)
	{
		numKeys = pastKeys + futureKeys + 1;
		numSamples = (pastKeys + futureKeys) * resolution + 1;
		int windowResolution = resolution * pastKeys; // FPS
		pivotIndex = windowResolution; // index after past window
		
	}

	virtual void IncrementSequence(int startIdx, int endIdx) = 0;

	void IncrementPastSequecne()
	{
		IncrementSequence(0, pivotIndex);
	}

	virtual void Interpolate(int startIdx, int endIdx) = 0;

};

#endif // !MATHUTILS_H