set(target_name ToyCollectionPlugin)

qt_add_library(${target_name}
    ToyCollectionPlugin_global.h
    ToyCollectionPlugin.h ToyCollectionPlugin.cpp 
    ToyAlphaNode.h ToyAlphaNode.cpp
    ToyBetaNode.h ToyBetaNode.cpp
    
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/NeuralNetwork
)

target_include_directories(${target_name} PUBLIC
)

target_compile_definitions(${target_name} PRIVATE
    TOYCOLLECTIONPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
 )

target_link_libraries(${target_name} PUBLIC
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
