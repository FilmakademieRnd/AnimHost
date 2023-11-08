qt_add_library(TestDataSourcePlugin
    TestDataSourcePlugin.cpp TestDataSourcePlugin.h
    TestDataSourcePlugin_global.h
)
target_include_directories(TestDataSourcePlugin PUBLIC
    ../PluginInterface
)

target_compile_definitions(TestDataSourcePlugin PRIVATE
    TESTDATASOURCEPLUGIN_LIBRARY
)

target_link_libraries(TestDataSourcePlugin PUBLIC
    PluginInterface
    AnimHostCore
)



