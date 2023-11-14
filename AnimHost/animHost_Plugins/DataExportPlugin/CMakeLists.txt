
qt_add_library(DataExportPlugin
    DataExportPlugin.cpp DataExportPlugin.h
    DataExportPlugin_global.h
)

set_target_properties (DataExportPlugin PROPERTIES
    FOLDER Plugins/DataOutput
)

target_include_directories(DataExportPlugin PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(DataExportPlugin PRIVATE
    DATAEXPORTPLUGIN_LIBRARY
)

target_link_libraries(DataExportPlugin PUBLIC
    PluginNodeInterface
)
