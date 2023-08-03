
#ifndef ONNXHELPER_H
#define ONNXHELPER_H

#include <onnxruntime_cxx_api.h>



class OnnxHelper {

public:

	template<typename T>
	static Ort::Value vecToTensor(std::vector<T>& data, const std::vector<std::int64_t>& shape) {
		Ort::MemoryInfo mem_info =
			Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
		auto tensor = Ort::Value::CreateTensor<T>(mem_info, data.data(), data.size(), shape.data(), shape.size());
		return tensor;
	}
};

#endif // BASICONNXPLUGINPLUGIN_H