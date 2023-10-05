
qt_add_library(DataExportPlugin
    DataExportPlugin.cpp DataExportPlugin.h
    DataExportPlugin_global.h
)
target_include_directories(DataExportPlugin PUBLIC
    ../PluginNodeInterface
    ../../../glm
)

target_compile_definitions(DataExportPlugin PUBLIC
    DATAEXPORTPLUGIN_LIBRARY
)

target_link_libraries(DataExportPlugin PUBLIC
    PluginNodeInterface
    AnimHostCore
)
