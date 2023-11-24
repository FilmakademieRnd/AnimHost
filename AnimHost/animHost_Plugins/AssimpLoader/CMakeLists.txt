qt_add_library(AssimpLoader
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

target_link_directories(AnimHostCore PUBLIC 
    ../../../assimp/lib/Debug
)

target_link_libraries(AssimpLoader PUBLIC
    PluginNodeInterface
    assimp-vc143-mtd
)



