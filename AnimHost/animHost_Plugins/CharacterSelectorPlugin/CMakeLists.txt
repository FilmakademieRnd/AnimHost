qt_add_library(CharacterSelectorPlugin
    CharacterSelectorPlugin.cpp CharacterSelectorPlugin.h
    CharacterSelectorPlugin_global.h
)

set_target_properties (CharacterSelectorPlugin PROPERTIES
    FOLDER Plugins/Operator
)

target_include_directories(CharacterSelectorPlugin PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(CharacterSelectorPlugin PRIVATE
    CHARACTERSELECTORPLUGIN_LIBRARY
)

target_link_libraries(CharacterSelectorPlugin PUBLIC
    PluginNodeInterface
    AnimHostCore
)
