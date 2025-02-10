set(target_name DeepLocomotionPlugin)
# Explicitly specify the paths to the local installation of Matplot++
# set(MATPLOTPP_INCLUDE_DIR "C:/Program Files (x86)/Matplot++/include")
# set(MATPLOTPP_LIBRARY_DIR "C:/Program Files (x86)/Matplot++/lib")

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
#    ${MATPLOTPP_INCLUDE_DIR}
)

target_compile_definitions(${target_name} PRIVATE
    DEEPLOCOMOTIONPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../BasicOnnxPlugin/onnxruntime/lib
#    ${MATPLOTPP_LIBRARY_DIR}
)

target_link_libraries(${target_name} PUBLIC
    BasicOnnxPlugin    
    AnimHostCore
)

target_link_libraries(${target_name} PRIVATE
    BasicOnnxPlugin    
    AnimHostCore
#    matplot
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)