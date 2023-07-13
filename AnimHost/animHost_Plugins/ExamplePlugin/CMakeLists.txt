qt_add_library(ExamplePlugin
    exampleplugin.cpp exampleplugin.h
    exampleplugin_global.h
)
target_include_directories(ExamplePlugin PUBLIC
    ../PluginInterface
    ../../core/QTNodes/include
    ../../../glm
)

target_compile_definitions(ExamplePlugin PUBLIC
    EXAMPLEPLUGIN_LIBRARY
)

target_link_libraries(ExamplePlugin PUBLIC
    Qt::Core
    Qt::Gui
    PluginInterface
)



