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

 
#ifndef ONNXTENSOR_H
#define ONNXTENSOR_H

#include  <commondatatypes.h>


class OnnxTensor {
	

	std::vector<float> tensorData;
	std::vector<std::int64_t> shape;

public:

	OnnxTensor();
	OnnxTensor(const std::vector<std::int64_t>& inShape);

	bool IsShapeEqual(const std::vector<std::int64_t>& rShape);

	COMMONDATA(tensor,Tensor)
};
Q_DECLARE_METATYPE(OnnxTensor)
Q_DECLARE_METATYPE(std::shared_ptr<OnnxTensor>)

#endif