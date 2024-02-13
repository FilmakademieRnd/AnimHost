
qt_add_library(ModeAdaptivePreprocessPlugin SHARED
    ModeAdaptivePreprocessPlugin.cpp ModeAdaptivePreprocessPlugin.h
    ModeAdaptivePreprocessPlugin_global.h
)
target_include_directories(ModeAdaptivePreprocessPlugin PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(ModeAdaptivePreprocessPlugin PRIVATE
    MODEADAPTIVEPREPROCESSPLUGIN_LIBRARY
)

target_link_libraries(ModeAdaptivePreprocessPlugin PUBLIC
    PluginNodeInterface
)
