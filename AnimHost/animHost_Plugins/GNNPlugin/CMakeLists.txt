
qt_add_library(GNNPlugin
    GNNPlugin.cpp GNNPlugin.h
    GNNController.cpp GNNController.h
    GNNPlugin_global.h
    HistoryBuffer.h
)
target_include_directories(GNNPlugin PUBLIC
    ../PluginNodeInterface
    ../BasicOnnxPlugin
    ../BasicOnnxPlugin/onnxruntime/include
    ../../../glm
)

set_target_properties (GNNPlugin PROPERTIES
    FOLDER Plugins/NeuralNetwork
)


target_compile_definitions(GNNPlugin PRIVATE
    GNNPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../BasicOnnxPlugin/onnxruntime/lib
)

target_link_libraries(GNNPlugin PUBLIC
    BasicOnnxPlugin    
    PluginNodeInterface
    AnimHostCore
)
