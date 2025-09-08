set(target_name TrainFrameworkPlugin)

qt_add_library(${target_name}
    TrainFrameworkPlugin.cpp TrainFrameworkPlugin.h
    TrainFrameworkPlugin_global.h
    TrainFramework/TrainFrameworkNode.h TrainFramework/TrainFrameworkNode.cpp

)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/Custom
)


target_include_directories(${target_name} PUBLIC
)

target_compile_definitions(${target_name} PRIVATE
    TRAINFRAMEWORKPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)