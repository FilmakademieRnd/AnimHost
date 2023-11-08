qt_add_library(AnimationFrameSelectorPlugin
    AnimationFrameSelectorPlugin.cpp AnimationFrameSelectorPlugin.h
    AnimationFrameSelectorPlugin_global.h
)

set_target_properties (AnimationFrameSelectorPlugin PROPERTIES
    FOLDER Plugins/Operator
)

target_include_directories(AnimationFrameSelectorPlugin PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(AnimationFrameSelectorPlugin PRIVATE
    ANIMATIONFRAMESELECTORPLUGIN_LIBRARY
)

target_link_libraries(AnimationFrameSelectorPlugin PUBLIC
    PluginNodeInterface
)
