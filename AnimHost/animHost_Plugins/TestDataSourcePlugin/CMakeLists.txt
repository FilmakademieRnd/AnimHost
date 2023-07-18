qt_add_library(TestDataSourcePlugin
    TestDataSourcePlugin.cpp TestDataSourcePlugin.h
    TestDataSourcePlugin_global.h
)
target_include_directories(TestDataSourcePlugin PUBLIC
    ../PluginInterface
    ../../core/QTNodes/include
    ../../../glm
)

target_compile_definitions(TestDataSourcePlugin PUBLIC
    TESTDATASOURCEPLUGIN_LIBRARY
)

target_link_libraries(TestDataSourcePlugin PUBLIC
    Qt::Core
    Qt::Gui
    PluginInterface
)



