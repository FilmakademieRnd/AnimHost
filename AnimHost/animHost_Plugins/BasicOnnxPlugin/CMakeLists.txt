qt_add_library(BasicOnnxPlugin
    BasicOnnxPlugin.cpp BasicOnnxPlugin.h
    OnnxModel.h OnnxModel.cpp
    OnnxTensor.h OnnxTensor.cpp
    OnnxModelViewWidget.cpp OnnxModelViewWidget.h
    BasicOnnxPlugin_global.h
    OnnxHelper.h
)

target_include_directories(BasicOnnxPlugin PUBLIC
    ../PluginNodeInterface
    ./onnxruntime/include
    ../../../glm
)

target_compile_definitions(BasicOnnxPlugin PUBLIC
    BASICONNXPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ./onnxruntime/lib
)

target_link_libraries(BasicOnnxPlugin PUBLIC
    PluginNodeInterface
    AnimHostCore
    onnxruntime
)
