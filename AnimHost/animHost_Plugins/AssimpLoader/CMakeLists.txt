set(target_name AssimpLoader)


qt_add_library(${target_name}
    assimploaderplugin.cpp assimploaderplugin.h
    assimploaderplugin_global.h
    assimphelper.h assimphelper.cpp
)

set_target_properties (AssimpLoader PROPERTIES
    FOLDER Plugins/DataInput
)

target_include_directories(AssimpLoader PUBLIC
    ../../../assimp/include
)

target_compile_definitions(AssimpLoader PRIVATE
    ASSIMPLOADERPLUGIN_LIBRARY
)

target_link_directories(${target_name} PUBLIC 
    ../../../assimp/lib/Debug
)

target_link_libraries(AssimpLoader PUBLIC
    PluginNodeInterface
    assimp-vc143-mtd
)


install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

