set(target_name PluginNodeInterface)

qt_add_library(${target_name}
    pluginnodeinterface.h pluginnodeinterface.cpp
    pluginnodeinterface_global.h
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins
)

target_compile_definitions(${target_name} PUBLIC
    PLUGINNODEINTERFACE_LIBRARY
)

target_include_directories(${target_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${target_name} PUBLIC
    ../../core/QTNodes/include
    ../PluginInterface
)

target_link_directories(${target_name} PRIVATE 
    ${CMAKE_SOURCE_DIR}/core/QTNodes/lib
)

target_link_libraries(${target_name} PUBLIC
    PluginInterface
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
