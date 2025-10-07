set(target_name DataExportPlugin)

qt_add_library(${target_name}
    DataExportPlugin.cpp DataExportPlugin.h
    DataExportPlugin_global.h
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/DataOutput
)

target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(${target_name} PRIVATE
    DATAEXPORTPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    PluginInterface
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
