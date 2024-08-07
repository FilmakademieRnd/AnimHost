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