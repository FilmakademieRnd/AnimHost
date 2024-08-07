set(target_name GNNPlugin)

qt_add_library(${target_name}
    GNNPlugin.cpp GNNPlugin.h
    GNNController.cpp GNNController.h
    GNNPlugin_global.h
    HistoryBuffer.h
    PhaseSequence.h PhaseSequence.cpp
    RootSeries.h RootSeries.cpp
)
target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
    ../BasicOnnxPlugin
    ../BasicOnnxPlugin/onnxruntime/include
    ../../../glm
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/NeuralNetwork
)

target_compile_definitions(${target_name} PRIVATE
    GNNPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../BasicOnnxPlugin/onnxruntime/lib
)

target_link_libraries(${target_name} PUBLIC
    BasicOnnxPlugin    
    PluginNodeInterface
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
