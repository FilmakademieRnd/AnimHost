qt_add_library(PluginNodeInterface
    pluginnodeinterface.h pluginnodeinterface.cpp
    pluginnodeinterface_global.h
)

set_target_properties (PluginNodeInterface PROPERTIES
    FOLDER Plugins
)

target_compile_definitions(PluginNodeInterface PUBLIC
    PLUGINNODEINTERFACE_LIBRARY
)

target_include_directories(PluginNodeInterface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(PluginNodeInterface PUBLIC
    ../../core/QTNodes/include
    ../PluginInterface
)

target_link_directories(PluginNodeInterface PRIVATE 
    ${CMAKE_SOURCE_DIR}/core/QTNodes/lib
)

target_link_libraries(PluginNodeInterface PUBLIC
    PluginInterface
)