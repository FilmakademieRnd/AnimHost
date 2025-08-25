set(target_name HelloWorldPlugin)

qt_add_library(${target_name}
    HelloWorldPlugin.cpp HelloWorldPlugin.h
    HelloWorldPlugin_global.h
    HelloWorld/HelloWorldNode.h HelloWorld/HelloWorldNode.cpp

)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/Custom
)


target_include_directories(${target_name} PUBLIC
)

target_compile_definitions(${target_name} PRIVATE
    HELLOWORLDPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)