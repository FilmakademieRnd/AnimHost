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

 
#include "OnnxTensor.h"

OnnxTensor::OnnxTensor()
{
	shape = { -1 };
	tensorData = { 0 };
}


OnnxTensor::OnnxTensor(const std::vector<std::int64_t>& inShape)
{
	shape = inShape;
	auto total_num_of_elements = std::accumulate(shape.begin(), shape.end(), 1,
		[](int a, int b) { return a * b; });

	tensorData = std::vector<float>(total_num_of_elements);

}


bool OnnxTensor::IsShapeEqual(const std::vector<std::int64_t>& rShape)
{
	bool equals = (shape.size() == rShape.size()) && std::equal(shape.begin(), shape.end(), rShape.begin());
	return equals;
}
