set(target_name TestDataSourcePlugin)

qt_add_library(${target_name}
    TestDataSourcePlugin.cpp TestDataSourcePlugin.h
    TestDataSourcePlugin_global.h
)
target_include_directories(${target_name} PUBLIC
    ../PluginInterface
)

target_compile_definitions(${target_name} PRIVATE
    TESTDATASOURCEPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    PluginInterface
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
