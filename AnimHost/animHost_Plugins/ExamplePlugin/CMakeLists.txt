set(target_name ExamplePlugin)

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
)


target_link_libraries(ExamplePlugin PUBLIC
    # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../PluginInterface/lib/
    PluginInterface
)




install(TARGETS ExamplePlugin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)