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
