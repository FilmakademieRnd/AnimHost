qt_add_library(ExamplePlugin
    exampleplugin.cpp exampleplugin.h
    exampleplugin_global.h
)
target_include_directories(ExamplePlugin PUBLIC
    ../PluginInterface
    ../../../glm
)

target_compile_definitions(ExamplePlugin PUBLIC
    EXAMPLEPLUGIN_LIBRARY
)

target_link_libraries(ExamplePlugin PUBLIC
    PluginInterface
    AnimHostCore
)



