qt_add_library(PluginInterface
    plugininterface.h plugininterface.cpp
    plugininterface_global.h
)

target_compile_definitions(PluginInterface PUBLIC
    PLUGININTERFACE_LIBRARY
)

target_include_directories(PluginInterface PUBLIC
    ../../../glm
    ../../core/QTNodes/include
)

target_link_libraries(PluginInterface PUBLIC
    Qt::Core
    Qt::Gui
)



