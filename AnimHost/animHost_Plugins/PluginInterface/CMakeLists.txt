set(target_name PluginInterface)

qt_add_library(${target_name}
    plugininterface.h plugininterface.cpp
    plugininterface_global.h
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins
)

target_compile_definitions(${target_name} PUBLIC
    PLUGININTERFACE_LIBRARY
)

target_include_directories(${target_name} PUBLIC
    ../../core/QTNodes/include
)

target_link_libraries(${target_name} PUBLIC
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
