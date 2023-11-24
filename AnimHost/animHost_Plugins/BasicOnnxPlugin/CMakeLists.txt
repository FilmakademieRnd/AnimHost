qt_add_library(BasicOnnxPlugin
    BasicOnnxPlugin.cpp BasicOnnxPlugin.h
    OnnxModel.h OnnxModel.cpp
    OnnxTensor.h OnnxTensor.cpp
    OnnxModelViewWidget.cpp OnnxModelViewWidget.h
    BasicOnnxPlugin_global.h
    OnnxHelper.h
)

set_target_properties (BasicOnnxPlugin PROPERTIES
    FOLDER Plugins/NeuralNetwork
)


target_include_directories(BasicOnnxPlugin PUBLIC
    ../PluginNodeInterface
    ./onnxruntime/include
)

target_compile_definitions(BasicOnnxPlugin PRIVATE
    BASICONNXPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ./onnxruntime/lib
)

target_link_libraries(BasicOnnxPlugin PUBLIC
    PluginNodeInterface
    onnxruntime
)
