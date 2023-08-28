#ifndef ONNXMODEl_H
#define ONNXMODEl_H

#include <QMetaType>
#include <onnxruntime_cxx_api.h>

class OnnxModel {


private:
    //ONNX
    std::unique_ptr<Ort::Session> session;
    std::unique_ptr<Ort::Env> environment;

    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<std::vector<std::int64_t>> input_shapes;
    std::vector<std::vector<std::int64_t>> output_shapes;

    QString OnnxModelFilePath = "";
    bool bModelValid = false;

public:
	OnnxModel();
	~OnnxModel() {};

    void SetupEnvironment();
    void LoadOnnxModel(QString Path);

    bool IsModelValid() { return bModelValid; };

    std::vector<std::string> GetTensorNames(bool bGetInput = true);
    std::vector<std::string> GetTensorShapes(bool bGetInput = true);

    unsigned int GetNumTensors(bool bGetInput = true) {

        if (bModelValid) {
            return bGetInput ? session->GetInputCount() : session->GetOutputCount();
        }
        
        return 0;
    }

    void RunInference();

private:
    std::string shape_printer(const std::vector<int64_t>& vec) {
        
        std::string s = std::accumulate(vec.begin(), vec.end(), std::string{},
            [](const std::string& a, int b) {
                return a.empty() ? std::to_string(b) : a + ", " + std::to_string(b);
            });
        return "[" + s + "]";
    }

};

#endif 