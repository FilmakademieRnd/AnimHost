
qt_add_library(HistoryPlugin
    HistoryPlugin.cpp HistoryPlugin.h
    HistoryPlugin_global.h
    HistoryHelper.h
)
target_include_directories(HistoryPlugin PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(HistoryPlugin PRIVATE
    HISTORYPLUGIN_LIBRARY
)

target_link_libraries(HistoryPlugin PUBLIC
    PluginNodeInterface

)
