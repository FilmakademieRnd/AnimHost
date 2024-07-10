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

 
#ifndef ONNXMODEl_H
#define ONNXMODEl_H
#include "BasicOnnxPlugin_global.h"

#include <QMetaType>
#include <onnxruntime_cxx_api.h>

class BASICONNXPLUGINSHARED_EXPORT OnnxModel {


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

    std::vector<float> RunInference(std::vector<float>& inputValue);

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