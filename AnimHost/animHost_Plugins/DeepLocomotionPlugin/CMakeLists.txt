set(target_name DeepLocomotionPlugin)

qt_add_library(${target_name}
    DeepLocomotionPlugin.cpp DeepLocomotionPlugin.h
    DeepLocomotionPlugin_global.h
    LocomotionPreprocess/LocomotionPreprocessNode.h LocomotionPreprocess/LocomotionPreprocessNode.cpp
    GNN/GNNNode.h GNN/GNNNode.cpp
    GNN/GNNController.h GNN/GNNController.cpp
    GNN/HistoryBuffer.h
    GNN/PhaseSequence.h GNN/PhaseSequence.cpp
    GNN/RootSeries.h GNN/RootSeries.cpp
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/Locomotion
)


target_include_directories(${target_name} PUBLIC
    ../BasicOnnxPlugin
    ../BasicOnnxPlugin/onnxruntime/include
    ../../../glm
)

target_compile_definitions(${target_name} PRIVATE
    DEEPLOCOMOTIONPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../BasicOnnxPlugin/onnxruntime/lib
)

target_link_libraries(${target_name} PUBLIC
    BasicOnnxPlugin    
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)