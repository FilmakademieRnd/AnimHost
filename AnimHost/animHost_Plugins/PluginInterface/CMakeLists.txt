qt_add_library(PluginInterface
    plugininterface.h plugininterface.cpp
    plugininterface_global.h
)

set_target_properties (PluginInterface PROPERTIES
    FOLDER Plugins
)

target_compile_definitions(PluginInterface PUBLIC
    PLUGININTERFACE_LIBRARY
)

target_include_directories(PluginInterface PUBLIC
    ../../../glm
    ../../../assimp/include
    ../../core/QTNodes/include
)

target_link_libraries(PluginInterface PUBLIC
    Qt::Core
    Qt::Gui
)



