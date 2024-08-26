set(target_name BasicOnnxPlugin)

qt_add_library(${target_name}
    BasicOnnxPlugin.cpp BasicOnnxPlugin.h
    OnnxModel.h OnnxModel.cpp
    OnnxTensor.h OnnxTensor.cpp
    OnnxModelViewWidget.cpp OnnxModelViewWidget.h
    BasicOnnxPlugin_global.h
    OnnxHelper.h
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/NeuralNetwork
)

target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
    ./onnxruntime/include
)

target_compile_definitions(${target_name} PUBLIC
    BASICONNXPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ./onnxruntime/lib
)

target_link_libraries(${target_name} PUBLIC
    PluginInterface
    onnxruntime
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    ./onnxruntime/lib/onnxruntime.dll
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)