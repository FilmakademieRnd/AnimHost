
qt_add_library(AnimationFrameSelectorPlugin
    AnimationFrameSelectorPlugin.cpp AnimationFrameSelectorPlugin.h
    AnimationFrameSelectorPlugin_global.h
)
target_include_directories(AnimationFrameSelectorPlugin PUBLIC
    ../PluginNodeInterface
    ../../../glm
)

target_compile_definitions(AnimationFrameSelectorPlugin PUBLIC
    ANIMATIONFRAMESELECTORPLUGIN_LIBRARY
)

target_link_libraries(AnimationFrameSelectorPlugin PUBLIC
    PluginNodeInterface
    AnimHostCore
)
