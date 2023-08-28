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