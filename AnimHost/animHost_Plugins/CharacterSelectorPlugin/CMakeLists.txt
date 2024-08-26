set(target_name CharacterSelectorPlugin)

qt_add_library(${target_name}
    CharacterSelectorPlugin.cpp CharacterSelectorPlugin.h
    CharacterSelectorPlugin_global.h
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/Operator
)

target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(${target_name} PRIVATE
    CHARACTERSELECTORPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    PluginInterface
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
