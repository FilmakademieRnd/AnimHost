
#include "BasicOnnxPlugin.h"
#include "OnnxHelper.h"
#include <QPushButton>
#include <onnxruntime_cxx_api.h>


BasicOnnxPlugin::BasicOnnxPlugin()
{
    _pushButton = nullptr;
    qDebug() << "BasicOnnxPlugin created";
}

BasicOnnxPlugin::~BasicOnnxPlugin()
{
    qDebug() << "~BasicOnnxPlugin()";
}

unsigned int BasicOnnxPlugin::nPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}

NodeDataType BasicOnnxPlugin::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}

void BasicOnnxPlugin::setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "BasicOnnxPlugin setInData";
}

std::shared_ptr<NodeData> BasicOnnxPlugin::outData(QtNodes::PortIndex port)
{
	return nullptr;
}

QWidget* BasicOnnxPlugin::embeddedWidget()
{
	if (!_pushButton) {
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &BasicOnnxPlugin::onButtonClicked);
	}

	return _pushButton;
}

void BasicOnnxPlugin::onButtonClicked()
{
	qDebug() << "Onnx Model Exploration";

    // Setup runtime
    Ort::Env env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "explore model");
    const wchar_t* modelFilepath = L"test.onnx";
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(1);
    Ort::Session session( env, modelFilepath, sessionOptions);

    //get input & output names
    
    Ort::AllocatorWithDefaultOptions allocator;

    std::vector<std::string> input_names;
    std::vector<std::int64_t> input_shapes;

    qDebug() << "Input Node Name/Shape(" << input_names.size() << "): \n";

    auto shape_printer = [](const std::vector<int64_t>& vec) {
        return std::accumulate(vec.begin(), vec.end(), std::string{},
                                [](const std::string& a, int b) {
                                        return a.empty() ? std::to_string(b) : a + " " + std::to_string(b);
                                });};

    for (std::size_t i = 0; i < session.GetInputCount(); i++) {
        input_names.emplace_back(session.GetInputNameAllocated(i, allocator).get());
        input_shapes = session.GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        qDebug() << "\t" << input_names.at(i) << shape_printer(input_shapes);
    }

    // output

    std::vector<std::string> output_names;
    std::vector<std::int64_t> output_shapes;

    for (std::size_t i = 0; i < session.GetOutputCount(); i++) {
        output_names.emplace_back(session.GetOutputNameAllocated(i, allocator).get());
        output_shapes = session.GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        qDebug() << "\t" << output_names.at(i) << shape_printer(output_shapes);
    }

    // gen random input tensor

    auto input_shape = input_shapes;
    auto total_num_of_elements = std::accumulate(input_shape.begin(), input_shape.end(), 1, 
                                                    [](int a, int b) { return a * b; });

    std::vector<float> input_tensor_values(total_num_of_elements);
    std::generate(input_tensor_values.begin(), input_tensor_values.end(), [&]() { return rand() % 255; });

    std::vector<Ort::Value> input_tensors;
    
    input_tensors.emplace_back(OnnxHelper::vecToTensor<float>(input_tensor_values, input_shape));
    qDebug() << "\ninput_tensor shape: " << shape_printer(input_tensors[0].GetTensorTypeAndShapeInfo().GetShape());

    //model inference
    std::vector<const char*> input_names_c(input_names.size(), nullptr);
    std::transform(input_names.begin(), input_names.end(), input_names_c.begin(),
        [&](const std::string& str) { return str.c_str(); });

    std::vector<const char*> output_names_c(output_names.size(), nullptr);
    std::transform(output_names.begin(), output_names.end(), output_names_c.begin(),
        [&](const std::string& str) { return str.c_str(); });

    qDebug() << "Start Model Inference ...";

    try {
        auto out_tensors = session.Run(Ort::RunOptions{ nullptr }, input_names_c.data(), input_tensors.data(),
            input_names_c.size(), output_names_c.data(), output_names_c.size());

        float ret = out_tensors[0].At<float>({0});

        qDebug() << "Result: " << output_names[0] << "\t" << ret;

        qDebug() <<"... Done!";
    }
    catch (const Ort::Exception& exception) {
        qDebug() << "ERROR running model inference: " << exception.what();
    }
}
