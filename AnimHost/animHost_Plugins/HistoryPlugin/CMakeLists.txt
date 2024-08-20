set(target_name HistoryPlugin)

qt_add_library(${target_name}
    HistoryPlugin.cpp HistoryPlugin.h
    HistoryPlugin_global.h
    HistoryHelper.h
)
target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(${target_name} PRIVATE
    HISTORYPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    PluginInterface

)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
