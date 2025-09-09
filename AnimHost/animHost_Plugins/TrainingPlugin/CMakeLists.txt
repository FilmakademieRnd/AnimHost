set(target_name TrainingPlugin)

qt_add_library(${target_name}
    TrainingPlugin.cpp TrainingPlugin.h
    TrainingPlugin_global.h
    Training/TrainingNode.h Training/TrainingNode.cpp
    Training/TrainingNodeWidget.h Training/TrainingNodeWidget.cpp

)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/Custom
)


target_include_directories(${target_name} PUBLIC
)

target_compile_definitions(${target_name} PRIVATE
    TRAININGPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)