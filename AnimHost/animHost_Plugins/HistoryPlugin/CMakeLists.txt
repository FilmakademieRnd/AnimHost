
qt_add_library(HistoryPlugin
    HistoryPlugin.cpp HistoryPlugin.h
    HistoryPlugin_global.h
    HistoryHelper.h
)
target_include_directories(HistoryPlugin PUBLIC
    ../PluginNodeInterface
    ../../../glm
)

target_compile_definitions(HistoryPlugin PUBLIC
    HISTORYPLUGIN_LIBRARY
)

target_link_libraries(HistoryPlugin PUBLIC
    PluginNodeInterface
    AnimHostCore
)
