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

 
#include "onnxModel.h"
#include "OnnxHelper.h"
#include <QtWidgets>
#include <onnxruntime_cxx_api.h>

OnnxModel::OnnxModel() 
{
    SetupEnvironment();
}

void OnnxModel::SetupEnvironment()
{
	// Setup runtime
	environment = std::make_unique<Ort::Env>(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "explore model");

}

bool OnnxModel::LoadOnnxModel(QString Path)
{
    std::wstring modelFilepath = Path.toStdWString();

    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(1);

    try {
        session = std::make_unique<Ort::Session>(*environment.get(), modelFilepath.c_str(), sessionOptions);
        bModelValid = true;

        Ort::AllocatorWithDefaultOptions allocator;

		QString networkReport = QString("Input Node Name/Shape:");

        for (std::size_t i = 0; i < session->GetInputCount(); i++) {

            input_names.push_back(session->GetInputNameAllocated(i, allocator).get());
            input_shapes.emplace_back(session->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape());
            networkReport = networkReport + " " + QString::fromStdString(input_names.at(i)) + QString::fromStdString(shape_printer(input_shapes[i]));
        }
		qInfo(networkReport.toLocal8Bit().data());


        // output
        networkReport = "Output Node Name/Shape:";
        for (std::size_t i = 0; i < session->GetOutputCount(); i++) {
            output_names.push_back(session->GetOutputNameAllocated(i, allocator).get());
            output_shapes.emplace_back(session->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape());
            networkReport = networkReport + " " + QString::fromStdString(output_names.at(i)) + QString::fromStdString(shape_printer(output_shapes[i]));
        }
        qInfo(networkReport.toLocal8Bit().data());
        return true;
    }

    catch (const Ort::Exception& exception) {
        qCritical() << "ERROR running model inference: " << exception.what();

        input_names.clear();
        input_shapes.clear();

        output_names.clear();
        output_shapes.clear();

        bModelValid = false;
        return false;
    }
    


}

std::vector<std::string> OnnxModel::GetTensorNames(bool bGetInput)
{
    if (bModelValid) {
        if (bGetInput) {
            return input_names;
        }
        else {
            return output_names;
        }
    }
    else {
        qDebug() << "Model not valid. No names to return.";
        return std::vector<std::string>();
    }
}

std::vector<std::string> OnnxModel::GetTensorShapes(bool bGetInput)
{
    if (bModelValid) {
            std::vector<std::string> retShape;
            auto shapes = bGetInput ? input_shapes : output_shapes;
            for (auto var : shapes) {
                retShape.emplace_back(shape_printer(var));
            }
            return retShape;
    }
    else {
        qDebug() << "Model not valid. No shapes to return.";
        return std::vector<std::string>();
    }
}

std::vector<float> OnnxModel::RunInference(std::vector<float>& inputValue)
{
    if (!bModelValid) {
        qCritical() << "Inference not possible. No model loaded or invalid.";
        return std::vector<float>(0);
    }

    std::vector<Ort::Value> input_tensors;


	//Dirty: RunInference only takes one vector reference, so technically only networks with one input tensor are supported
	//@TODO: Implement support for multiple input tensors
    for (std::size_t i = 0; i < session->GetInputCount(); i++) {
        auto total_num_of_elements = std::accumulate(input_shapes[i].begin(), input_shapes[i].end(), 1,
            [](int a, int b) { return a * b; });

        if (total_num_of_elements == inputValue.size()) {
            input_tensors.push_back(OnnxHelper::vecToTensor<float>(inputValue, input_shapes[i]));
        }
        else {
            qCritical() << "Inference not possible. Mismatch between input and network dimensions.";
			qCritical() << "Expected " << total_num_of_elements << " but got " << inputValue.size();
            return std::vector<float>(0);
        }
    }

   // qDebug() << "\ninput_tensor shape: " << shape_printer(input_tensors[0].GetTensorTypeAndShapeInfo().GetShape());

    std::vector<const char*> input_names_c(input_names.size(), nullptr);
    std::transform(input_names.begin(), input_names.end(), input_names_c.begin(),
        [&](const std::string& str) { return str.c_str(); });

    std::vector<const char*> output_names_c(output_names.size(), nullptr);
    std::transform(output_names.begin(), output_names.end(), output_names_c.begin(),
        [&](const std::string& str) { return str.c_str(); });

    auto in_names = input_names_c.data();
    auto in_tensor = input_tensors.data();
    float* ret = input_tensors[0].GetTensorMutableData<float>();
    auto in_size = input_names_c.size();
    auto out_names = output_names_c.data();
    auto out_size = output_names_c.size();


    try {
        auto out_tensors = session->Run(Ort::RunOptions{ nullptr }, in_names, in_tensor,
            in_size, out_names, out_size);

        ret = out_tensors[0].GetTensorMutableData<float>();
        auto a = out_tensors[0].GetTensorTypeAndShapeInfo().GetShape();


        //qDebug() << "Result: " << output_names[0] << "\t" << ret[30];

        auto out_num_of_elements = std::accumulate(a.begin(), a.end(), 1,
            [](int a, int b) { return a * b; });

        /*for (int i = 0; i < out_num_of_elements; i++) {
            qDebug() << ret[i];
        }*/

        //qDebug() << "... Done!";

        return std::vector<float>(ret, ret + out_num_of_elements);
    }
    catch (const Ort::Exception& exception) {
        qDebug() << "ERROR running model inference: " << exception.what();
        return std::vector<float>(0);
    }


}


void OnnxModel::RunInference() {
    //gen random input tensor

    if (!bModelValid) {
        qDebug() << "Inference not possible. No model loaded or invalid.";
        return;
    }

    std::vector<Ort::Value> input_tensors;
    std::vector<std::vector<float>> input_tensor_values;

    for (std::size_t i = 0; i < session->GetInputCount(); i++) {
        auto total_num_of_elements = std::accumulate(input_shapes[i].begin(), input_shapes[i].end(), 1,
            [](int a, int b) { return a * b; });

        input_tensor_values.push_back(std::vector<float>(total_num_of_elements));
        std::generate(input_tensor_values[i].begin(), input_tensor_values[i].end(), [&]() { return rand() % 255; });
        input_tensors.push_back(OnnxHelper::vecToTensor<float>(input_tensor_values[i], input_shapes[i]));

    }

    //input_tensors.emplace_back(OnnxHelper::vecToTensor<float>(input_tensor_values, input_shape));
    qDebug() << "\ninput_tensor shape: " << shape_printer(input_tensors[0].GetTensorTypeAndShapeInfo().GetShape());

    //model inference
    std::vector<const char*> input_names_c(input_names.size(), nullptr);
    std::transform(input_names.begin(), input_names.end(), input_names_c.begin(),
        [&](const std::string& str) { return str.c_str(); });

    std::vector<const char*> output_names_c(output_names.size(), nullptr);
    std::transform(output_names.begin(), output_names.end(), output_names_c.begin(),
        [&](const std::string& str) { return str.c_str(); });

    qDebug() << "Start Model Inference ...";

    auto in_names = input_names_c.data();
    auto in_tensor = input_tensors.data();
    float* ret = input_tensors[0].GetTensorMutableData<float>();
    auto in_size = input_names_c.size();
    auto out_names = output_names_c.data();
    auto out_size = output_names_c.size();

    //    ret = input_tensors[1].GetTensorMutableData<float>();

    try {
        auto out_tensors = session->Run(Ort::RunOptions{ nullptr }, in_names, in_tensor,
            in_size, out_names, out_size);

        ret = out_tensors[0].GetTensorMutableData<float>();
        auto a = out_tensors[0].GetTensorTypeAndShapeInfo().GetShape();


        qDebug() << "Result: " << output_names[0] << "\t" << ret[30];

        qDebug() << "... Done!";
    }
    catch (const Ort::Exception& exception) {
        qDebug() << "ERROR running model inference: " << exception.what();
    }
}
